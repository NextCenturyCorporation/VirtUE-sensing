#!/usr/bin/env python3

# Copyright (C) Two Six Labs, 2018
# Copyright (C) Matt Leinhos, 2018

"""
Sensors that wrap certain functionality in the bcc toolsuite
https://www.tecmint.com/bcc-best-linux-performance-monitoring-tools/

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


#msg_queue = curio.Queue() # JSON-compliant dicts produced by bcc tools and consumed by msg_consumer()

# Mapping: sensor_id --> curio.Queue() 
msg_queues = dict()


async def msg_consumer(message_stub, config=None, outqueue=None):
    """ Message consume that runs on behalf of every sensor """
    
    sensor_id = message_stub['sensor_id']
    sensor_name = message_stub['sensor_name']
    msg_queue = msg_queues[sensor_id]

    logging.debug("Running msg_consumer() for sensor %s, id %s",
                  message_stub['sensor_name'], sensor_id)
    async for inmsg in msg_queue:
        logging.debug("%s: dequeued %s from msg_queue",
                      sensor_name, inmsg)

        # Entire inbound JSON message is stuffed into "message" value of outbound message
        outdict = { "timestamp" : datetime.datetime.now().isoformat(),
                    "message"   : inmsg }
        if message_stub:
            outdict.update(message_stub)

        out_str = json.dumps(outdict)
        logging.debug("Enqueuing JSON message %s", out_str)            

        if not standalone:
            logging.debug("Enqueuing JSON message %s", out_str)
            await outqueue.put( out_str + "\n" )
            logging.debug("JSON message put in queue")


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
        """ """
        async with curio.TaskGroup() as t:
            for s in BccTool.all_sensors:
                await t.spawn(s.run)
    
    def __init__(self, name, cmd):
        BccTool.all_sensors.append(self)

        self.cmd = [ os.path.join(TOOLDIR, cmd), ]
        self.name = name
        self.header = None
        self.wrapper = None
        self.proc = None
        self.uuid = str(uuid.uuid4())

        # Create message queue for the producer/consumer
        self.msg_queue = curio.Queue()
        msg_queues[self.uuid] = self.msg_queue

        if standalone:
            self.wrapper_thread = threading.Thread(None,
                                                   standalone_run_msg_consumer,
                                                   "Sensor %s start thread".format(self.name),
                                                   args=(self.name, self.uuid))
        else:
            # The SensorWrapper will invoke msg_consumer, which in
            # turn will look up the correct queue based on the
            # sensor's uuid (sensor_id)
            self.wrapper = sensor_wrapper.SensorWrapper(name,
                                                        sensing_methods=[msg_consumer,],
                                                        parse_opts=False)

            # Kick off the sensor wrapper in its own thread
            logging.debug("Loading configuration data for sensor %s", self.name)
            paramdict = self._load_config_data(sensor_name)  # load the configuration data
            logger.info("loaded config data for sensor %s", sensor_id)
            paramdict['sensor_id'] = self.sensor_id  # artificially inject the sensor id
            paramdict['sensor_hostname'] = None  # artificially inject the sensor hostname
            paramdict['check_for_long_blocking'] = True
            logger.info("About to start the %s sensor...", sensor_name)

            # TODO: this is insufficient. We must parse the actual
            # command-line args and then relay them to the sensor
            # wrapper!!! See run_bcc_sensor.py
            self.wrapper_thread = threading.Thread(None,
                                                   self.wrapper.start,
                                                   "Sensor %s start thread".format(self.name),
                                                   args=(), kwargs=paramdict)
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
        logging.info("Running %s", cmd)

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
            logging.debug("Sensor %s putting %r on queue", self.name, d)
            await self.msg_queue.put(d)

        err = await self.stderr.readall()
        if err:
            logging.error("Error output: %s", err.decode())


def monitor_tools(standalone=False):
    #biosnoop  dcsnoop  execsnoop  killsnoop  mountsnoop  opensnoop  statsnoop  syncsnoop  ttysnoop
    #tcpaccept  tcpconnect

    tools = [ BccTool("exec", "execsnoop"),
              BccTool("kill", "killsnoop"),
              BccTool("open", "opensnoop"),
              BccTool("tcp-inbound", "tcpaccept"),
              BccTool("tcp-outbound", "tcpconnect"),
    ]

    #for t in tools:
    #    await t.run()

    #await BccTool.run_all_sensors()

    producer_thread = threading.Thread(None,
                                       curio.run,
                                       "Sensor producer thread",
                                       args=(BccTool.run_all_sensors,))
    producer_thread.run()

if __name__ == "__main__":
    if os.getenv('VERBOSE'):
        logging.basicConfig( level=logging.DEBUG )
    else:
        logging.basicConfig( level=logging.INFO )

    monitor_tools()
    #curio.run(monitor_tools)

