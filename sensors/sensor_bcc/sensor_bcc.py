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
 * enqueues that dictionary into a global queue (serving as event producer)

Each sensor also causes the event consumer (msg_consumer) to run. In
standalone mode, it does instructs curio to start it, and passes in
minimal sensor data.  In case we're in production mode, each sensor
creates its own instance of SensorWrapper, which in turn invokes the
coroutine msg_consumer. This way events are sent to the API server and
the coroutine is invoked with the correct sensor data.

There is one message queue per sensor. The queues are maintained in a global 
dictionary and can be referenced by the sensor's UUID.

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
import pdb

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

#from curio import subprocess, sleep
#from sensor_wrapper import SensorWrapper, report_on_file, which_file

TOOLDIR = '/usr/share/bcc/tools'

# List of supported BCC tools, in tuple (sensor-name, bcc-tool-name)
TOOLS = [ ("block-io",     "biosnoop" ),
          ("exec",         "execsnoop"),
          ("kill",         "killsnoop"),
          ("open",         "opensnoop"),
          ("shared-mem",   "shmsnoop" ),
          ("stat",         "statsnoop"),
          ("sync",         "syncsnoop"),
          ("tcp-inbound",  "tcpaccept"),
          ("tcp-outbound", "tcpconnect"), ]

#msg_queue = curio.Queue() # JSON-compliant dicts produced by bcc tools and consumed by msg_consumer()

# Mapping: sensor_id --> curio.Queue() 
msg_queues = dict()


async def msg_consumer(message_stub, config=None, outqueue=None):
    """ Message consume that runs on behalf of every sensor """
    
    sensor_id   = message_stub['sensor_id']
    sensor_name = message_stub['sensor_name']
    msg_queue   = msg_queues[sensor_id]

    logger.debug("Running msg_consumer() for sensor %s, id %s",
                  message_stub['sensor_name'], sensor_id)
    async for inmsg in msg_queue:
        logger.debug("[%s] dequeued '%r' from msg_queue and relaying",
                      sensor_name, inmsg)

        # Entire inbound JSON message is stuffed into "message" value of outbound message
        outdict = { "timestamp" : datetime.datetime.now().isoformat(),
                    "message"   : inmsg }

        outdict.update(message_stub)

        out_str = json.dumps(outdict)

        if not standalone:
            logger.debug("Enqueuing JSON message %s", out_str)
            await outqueue.put( out_str + "\n" )
            logger.debug("JSON message put in queue")


def standalone_run_msg_consumer(sensor_name, sensor_id):
    stub = { 'sensor_id' : sensor_id,
             'sensor_name' : sensor_name }
    curio.run(msg_consumer, stub)


class BccTool(object):

    # List of all the sensors, so we can select against their stdout pipes
    # ?????????????
    all_sensors = list()

    # ?????????????
    @staticmethod
    async def run_all_sensors():
        """ Run all the sensors in a TaskGroup"""
        async with curio.TaskGroup() as g:
            if standalone:
                # In standalone mode, kick of the single run of msg_consumer()
                pass #await g.spawn(msg_consumer)
            # In both modes, kick off all the sensors' run() methods
            for s in BccTool.all_sensors:
                logger.debug("Spawning %s", s.name)
                await g.spawn(s.run)
    
    def __init__(self, name, cmd):
        BccTool.all_sensors.append(self)

        self.cmd     = [ os.path.join(TOOLDIR, cmd), ]
        self.name    = name
        self.header  = None
        self.wrapper = None
        self.proc    = None
        self.uuid    = str(uuid.uuid4())

        # Create message queue for the producer/consumer
        self.msg_queue = curio.Queue()
        msg_queues[self.uuid] = self.msg_queue

        # Create a message consumer: we need msg_consumer() to run,
        # either via SensorWrapper or a thread we create here.
        if standalone:
            self.wrapper_thread = threading.Thread(None,
                                                   standalone_run_msg_consumer,
                                                   "Sensor %s start thread".format(self.name),
                                                   args=(self.name, self.uuid))
        if not standalone:
            # The SensorWrapper will invoke msg_consumer, which in
            # turn will look up the correct queue based on the
            # sensor's uuid (sensor_id). Each sensor wrapper is run in
            # its own thread.
            self.wrapper = sensor_wrapper.SensorWrapper(name,
                                                        sensing_methods=[msg_consumer,],
                                                        parse_opts=False)

            logger.debug("Loading configuration data for sensor %s", self.name)
            import pdb;pdb.set_trace()
            args = sys.argv
            opts = argparser.parse_args()
            opts.sensor_id = self.sensor_id # artificially inject sensor_id

            #paramdict = self._load_config_data(sensor_name)  # load the configuration data
            #paramdict['sensor_id'] = self.sensor_id  # artificially inject the sensor id
            #paramdict['sensor_hostname'] = None  # artificially inject the sensor hostname
            #paramdict['check_for_long_blocking'] = True
            logger.info("About to start the %s sensor...", sensor_name)

            # TODO: this is insufficient. We must parse the actual
            # command-line args and then relay them to the sensor
            # wrapper!!! See run_bcc_sensor.py
            self.wrapper_thread = threading.Thread(None,
                                                   self.wrapper.start,
                                                   "Sensor %s start thread".format(self.name),
                                                   args=opts, kwargs=paramdict)
        self.wrapper_thread.start()

    def __del__(self):
        if self.proc:
            self.proc.terminate()
        if self.wrapper_thread and self.wrapper_thread.isAlive():
            # TODO: signal the thread
            self.wrapper_thread.join(1.0)

    def parseline(self, line):
        items = line.decode("utf-8").split()

        # process header line, e.g. ['PCOMM', 'PID', 'PPID', 'RET', 'ARGS']
        if not self.header:
            self.header = items
            return None

        # process data line; by default merge extreneous data into final element
        # e.g. ['cat', '26929', '26016', '0', '/bin/cat', 'test'] ==>
        #      ['cat', '26929', '26016', '0', '/bin/cat test']
        if len(items) > len(self.header):
            items[len(self.header)-1] = ' '.join(items[len(self.header)-1:])

        # zip into dict, e.g. { 'PCOMM' : cat, 'PID' : 26929, ...}
        return dict(zip(self.header, items))

    async def run(self):
        # Spawn the tool so we can monitor stdout
        cmd = ' '.join(self.cmd)
        logger.info("Running %s", cmd)

        #self.proc = subprocess.Popen(cmd,
        #                             stdout=subprocess.PIPE,
        #                             stderr=subprocess.PIPE)
        self.proc = curio.subprocess.Popen(cmd,
                                           stdout=curio.subprocess.PIPE,
                                           stderr=curio.subprocess.PIPE)

        #self.proc = await asyncio.create_subprocess_exec(cmd,
        #                                                 stdout=asyncio.subprocess.PIPE,
        #                                                 stderr=asyncio.subprocess.PIPE)
        #self.stdout, self.stderr = await proc.communicate()
        self.stdout, self.stderr = self.proc.stdout, self.proc.stderr

        # TODO: kick off stderr reader
        async for line in self.stdout:
            d = self.parseline(line)
            if not d:
                continue
            logger.debug("[%s] enqueued '%r'", self.name, d)
            await self.msg_queue.put(d)

        err = await self.stderr.readall()
        if err:
            logger.error("Error output: %s", err.decode())


def monitor_tools(standalone=False):
    #biosnoop  dcsnoop  execsnoop  killsnoop  mountsnoop  opensnoop  statsnoop  syncsnoop  ttysnoop
    #tcpaccept  tcpconnect

    tools = [ BccTool(name,tool) for name,tool in TOOLS]
    #tools = [ BccTool("exec", "execsnoop"),
    #          BccTool("kill", "killsnoop"),
    #          BccTool("open", "opensnoop"),
    #          BccTool("tcp-inbound", "tcpaccept"),
    #          BccTool("tcp-outbound", "tcpconnect"),
    #]

    #for t in tools:
    #    await t.run()

    #await BccTool.run_all_sensors()

    producer_thread = threading.Thread(None,
                                       curio.run,
                                       "Sensor producer thread",
                                       args=(BccTool.run_all_sensors,))
    producer_thread.run()

if __name__ == "__main__":


    logger = logging.getLogger(__name__)
    #logger.addHandler(logging.NullHandler())

    # create logger
    #logger = logging.getLogger('simple_example')
    #logger.setLevel(logging.DEBUG)

    # create console handler and set level to debug
    ch = logging.StreamHandler(sys.stdout)
    if os.getenv('VERBOSE'):
        #logging.basicConfig( level=logging.DEBUG )
        ch.setLevel(logging.DEBUG)
        logger.setLevel(logging.DEBUG)
    else:
        ch.setLevel(logging.INFO)
    #ch.setLevel(logging.DEBUG)

    # create formatter
    formatter = logging.Formatter('%(asctime)s: %(message)s')

    # add formatter to ch
    ch.setFormatter(formatter)

    # add ch to logger
    logger.addHandler(ch)
    logger.info('test')

    '''
    if os.getenv('VERBOSE'):
        #logging.basicConfig( level=logging.DEBUG )
        logger.setLevel(logging.DEBUG)
    else:
        #loging.basicConfig( level=logging.INFO )
        logger.setLevel(logging.INFO)
    '''
    
    monitor_tools()
    #curio.run(monitor_tools)

