#!/usr/bin/env python3

# Copyright (C) Two Six Labs, 2019
# Copyright (C) Matt Leinhos, 2019

"""
Sensors that wrap a subset of the functionality in the bcc toolsuite
https://www.tecmint.com/bcc-best-linux-performance-monitoring-tools/

This script supports two modes of operation: standalone and
production. The standalone mode is intended for dev/debug use only, and 
does not relay messages to the sensor API server.

Each sensor:
 * spawns a supported bcc tool in a separate process
 * asynchronously reads the tool's output, one line at a time
 * packages the output into a dictionary
 * enqueues that dictionary into a single global queue (serving as event producer)

The event consumer runs in a separate thread.  In standalone mode, it
instructs curio to start it, and passes in minimal sensor data.  In
production mode, this sensor creates a single instance of
SensorWrapper, which in turn invokes the coroutine msg_consumer. This
way events are sent to the API server and the coroutine is invoked
with the correct sensor data.

There is one global message queue, which provided via curio. Thus it
is not thread-aware and must be protected.

"""

"""
To install bcc on Ubuntu 18.04 (and 16.04):

sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 4052245BD4284CDD
echo "deb https://repo.iovisor.org/apt/$(lsb_release -cs) $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/iovisor.list
sudo apt-get update
sudo apt-get install bcc-tools libbcc-examples linux-headers-$(uname -r)
"""


import os
import sys
import subprocess
import platform
import socket

import argparse
import logging
import re
import json
import datetime
import uuid

import curio
import curio.subprocess
import asyncio
import selectors

import threading

standalone = os.getenv('STANDALONE')
if not standalone:
    import sensor_wrapper

# Where do the BCC tools reside?
TOOLDIR = '/usr/share/bcc/tools'

# List of supported BCC tools, in tuple (sensor-name, bcc-tool-name, verbosity level)

# The priorty level is imposed externally via the sensor API. We emit
# only events from tools with an equal/higher level than the API has
# given the overall bcc sensor.

PRIORITY_LEVEL_OFF         = 0 # message never emitted
PRIORITY_LEVEL_LOW         = 1 # message emitted only at low level (i.e. low verbosity)
PRIORITY_LEVEL_DEFAULT     = 2
PRIORITY_LEVEL_HIGH        = 3
PRIORITY_LEVEL_ADVERSARIAL = 4 # message emitted at all non-zero levels (i.e. high verbosity)

#          sensor-name     bcc-tool      verbosity
TOOLS = [ ("block-io",     "biosnoop" ,  PRIORITY_LEVEL_DEFAULT),
          ("exec",         "execsnoop",  PRIORITY_LEVEL_DEFAULT),
          ("kill",         "killsnoop",  PRIORITY_LEVEL_DEFAULT),
          ("open",         "opensnoop",  PRIORITY_LEVEL_DEFAULT),
          ("shared-mem",   "shmsnoop" ,  PRIORITY_LEVEL_HIGH),
          ("stat",         "statsnoop",  PRIORITY_LEVEL_LOW),
          ("sync",         "syncsnoop",  PRIORITY_LEVEL_DEFAULT),
          ("tcp-inbound",  "tcpaccept",  PRIORITY_LEVEL_HIGH),
          ("tcp-outbound", "tcpconnect", PRIORITY_LEVEL_HIGH), ]

# JSON-compliant dicts are produced by the bcc tools and consumed by
# msg_consumer(). 
msg_queue = curio.Queue()

# Multiple producers enqueue items onto this queue, so protect it with
# a thread-aware locking mechanism. Items are taking off the queue by
# a single thread (no locking needed).
msg_queue_lock = threading.Lock()

msg_ct = 0

async def stop_request_callback():
    logging.warning("Stop requested")

async def dummy_wait():
    while True:
        await curio.sleep(5)

async def msg_consumer(message_stub, config=None, outqueue=None):
    """ Message consume that runs on behalf of every sensor """

    global msg_ct

    sensor_id   = message_stub['sensor_id']
    sensor_name = message_stub['sensor_name']

    logger.info("Running msg_consumer() for sensor '%s', id '%s'",
                message_stub['sensor_name'], sensor_id)

    async for inmsg in msg_queue:
        msg_ct += 1
        if msg_ct % 1000 == 0:
            logger.info("Handling event #%d", msg_ct)

        # Skip if external verbosity is lower than this event's?
        if config['prio'] < inmsg['prio']:
            continue
    
        logger.debug("[%s] dequeued '%r' from msg_queue and relaying",
                     inmsg['toolname'], inmsg)

        # Entire inbound JSON message is stuffed into "message" value of outbound message
        outdict = { "timestamp" : datetime.datetime.now().isoformat(),
                    "level"     : "info",
                    "message"   : inmsg }

        outdict.update(message_stub)

        out_str = json.dumps(outdict)
        logger.debug("Enqueuing JSON message %s", out_str)

        if not standalone:
            await outqueue.put( out_str + "\n" )

def standalone_run_msg_consumer(sensor_name, sensor_id):
    stub = { 'sensor_id' : sensor_id,
             'sensor_name' : sensor_name }
    logging.info("Starting msg_consumer() via curio library")
    curio.run(msg_consumer, stub)

class BccTool(object):

    # List of all the sensors, so curio can run async code
    # (i.e. select) against their stdout pipes
    all_sensors = list()

    @staticmethod
    async def run_all_sensors(*args):
        """ Run all the sensors in a TaskGroup """
        async with curio.TaskGroup() as g:
            # Kick off all the sensors' run() methods
            for s in BccTool.all_sensors:
                logger.info("Spawning runner for %s", s.name)
                await g.spawn(s.run)
    
    def __init__(self, name, cmd, prio):
        BccTool.all_sensors.append(self)

        self.cmd     = [ os.path.join(TOOLDIR, cmd), ]
        self.name    = name
        self.prio   = prio
        self.header  = None
        self.proc    = None

    def __del__(self):
        if self.proc:
            self.proc.terminate()

    def parseline(self, line):
        """ 
        Parse one line of bcc tool stdout. The first line is the header,
        in which case this method returns None.
        """

        items = line.decode("utf-8").split()

        # process header line, e.g. ['PCOMM', 'PID', 'PPID', 'RET', 'ARGS']
        if not self.header:
            # keys are lowercase, but values are passed as-is
            self.header = [x.lower() for x in items]
            return None

        # process data line; by default merge extreneous data into final element
        # e.g. ['cat', '26929', '26016', '0', '/bin/cat', 'test'] ==>
        #      ['cat', '26929', '26016', '0', '/bin/cat test']
        if len(items) > len(self.header):
            items[len(self.header)-1] = ' '.join(items[len(self.header)-1:])

        # zip into dict, e.g. { 'PCOMM' : cat, 'PID' : 26929, ...},
        # and add tool name
        d = dict(zip(self.header, items))
        d['toolname'] = self.name
        d['prio']     = self.prio
        return d

    async def run(self):
        # Spawn the tool so we can monitor stdout
        cmd = ' '.join(self.cmd)
        logger.info("Running %s", cmd)

        self.proc = curio.subprocess.Popen(cmd,
                                           stdout=curio.subprocess.PIPE,
                                           stderr=curio.subprocess.PIPE)

        self.stdout, self.stderr = self.proc.stdout, self.proc.stderr

        # TODO: kick off stderr reader
        async for line in self.stdout:
            d = self.parseline(line)
            if not d:
                continue

            logger.debug("[%s] enqueued '%r'", self.name, d)

            # Hold thread-aware lock to protect thread-unaware Queue
            with msg_queue_lock:
                await msg_queue.put(d)

        err = await self.stderr.readall()
        if err:
            logger.error("Error output: %s", err.decode())


def monitor_tools():
    # Start the bcc tools themselves. They're run in a separate
    # thread, and managed by curio within that thread.
    tools = [ BccTool(name,tool,prio) for name,tool,prio in TOOLS]

    producer_thread = threading.Thread(None,
                                       curio.run,
                                       "Sensor producer thread",
                                       args=(BccTool.run_all_sensors,))
    producer_thread.start()

    # Start the message consumer in its own thread, possibly from within SensorWrapper.
    if standalone:
        consumer_thread = threading.Thread(None,
                                           standalone_run_msg_consumer,
                                           "Standalone message consumer for sensor",
                                           args=("bcc", str(uuid.uuid4())))
    else:
        wrapper = sensor_wrapper.SensorWrapper("bcc",
                                               sensing_methods=[msg_consumer,],
                                               stop_notification=dummy_wait )
        kw = dict(check_for_long_blocking = True)
        consumer_thread = threading.Thread(None,
                                           wrapper.start,
                                           "BCC sensor wrapper thread",
                                           kwargs=kw)
    logger.info("Starting consumer thread")
    consumer_thread.start()


if __name__ == "__main__":
    logger = logging.getLogger(__name__)

    # create console handler and set level to debug
    ch = logging.StreamHandler(sys.stdout)
    fmt = logging.Formatter('%(asctime)s: %(message)s')
    ch.setFormatter(fmt)

    if os.getenv('VERBOSE'):
        #logging.basicConfig( level=logging.DEBUG )
        ch.setLevel(logging.DEBUG)
        logger.setLevel(logging.DEBUG)
    else:
        ch.setLevel(logging.INFO)
        logger.setLevel(logging.INFO)

    logger.addHandler(ch)

    monitor_tools()


