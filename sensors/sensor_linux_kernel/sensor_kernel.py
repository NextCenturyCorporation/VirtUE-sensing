#!/usr/bin/env python3

# Copyright (C) Two Six Labs, 2019
# Copyright (C) Matt Leinhos, 2019

"""
Script that reads and forwards data from the Linux kernel sensor socket.

This script merges output from all the kernel sensors into a single 
stream to the API.

In the alternate, the script can be run in STANDALONE mode. In this
case, it just reads the telemetry from the kernel sensor and
optionally prints info about it.

"""

import os
import sys

import logging
import datetime
import configparser
import argparse
import uuid
import json

import asyncio
import curio
import selectors

__VERSION__ = "1.20190103"
__MODULE__ = __file__

VIRTUE_PROTO_VERSION = 0.1
SOCKET_TIMEOUT = 1
UNIX_SOCK_PATH = '/var/run/kernel_sensor'

# Connection to UNIX domain socket
conn = None

# List of sensors that the kernel module is advertising
sensors = list()

standalone = os.getenv('STANDALONE')
if not standalone:
    import sensor_wrapper

verbose = os.getenv('VERBOSE') == '1'

logger = logging.getLogger(__name__)
#logger.addHandler(logging.NullHandler())


def parse_json(json_str):
    """ Parse one JSON message in given string. Return (json_obj, rest_of_string) """
    try:
        # N.B. the module gives us NULL characters, which we don't want
        dline = json_str.decode("utf-8").replace('\x00','')
    except UnicodeDecodeError as e:
        logger.warning("Failed to decode message [%s...]: %s", json_str, e)
        return None

    try:
        j = json.loads(dline)
    except json.decoder.JSONDecodeError as e:
        logger.info("Failed to parse JSON from message [%s...]: %s", dline, e)
        return None

    return j


def convert_sensor_reply(injson):
    """ The sensors output their data in different formats. Account for that here. """
    reply = injson.get('reply', None)
    if not reply or len(reply) < 4:
        return None

    sensor = reply[2]
    payload = { 'sensor' : sensor, 'data' : 'unhandled' }

    if sensor == 'kernel-lsof':
        payload['data'] = reply[4]
    elif sensor == 'kernel-ps':
        payload['data'] = reply[5]

    return payload    


async def send_json_obj(json_obj):
    msg = json.dumps(json_obj, sort_keys=True) + "\n"
    await conn.sendall(str.encode(msg))


async def recv_json_obj():
    line = await conn.as_stream().readline()
    if not line:
        return None

    obj = parse_json(line)

    return obj


async def fetch_sensor_records(sensor):
    request = {'Virtue-protocol-version': VIRTUE_PROTO_VERSION,
               'request': [ uuid.uuid4().hex, 'records', sensor['uuid']]}

    await send_json_obj(request)
    stream = conn.as_stream()
    
    msgs = list()
    try:
        async with curio.timeout_after(SOCKET_TIMEOUT):
            while True:
                resp = await stream.readline()
                if not resp:
                    break
                j = parse_json(resp)
                payload = convert_sensor_reply(j)
                if not payload:
                    break
                msgs.append(payload)
    except curio.TaskTimeout as e:
        pass

    logger.debug("Kernel sensor %s returned %d records", sensor['name'], len(msgs))
    return msgs


async def poll_sensors(message_stub = {}, config = {}, message_queue=None):
    """ Message consumer that constantly polls the kernel sensors """

    repeat_delay = config.get("repeat-interval", 15)
    print(" ::starting poll_sensors() for Linux kernel sensor relay")
    print("    $ repeat-interval = %d" % (repeat_delay,))

    while True:
        for s in sensors:
            records = await fetch_sensor_records(s)
            for r in records:
                psp_logmsg = {
                    "timestamp": datetime.datetime.now().isoformat(),
                    "level": "info",
                    "message": r }

                if message_stub:
                    psp_logmsg.update(message_stub)
                    await message_queue.put(json.dumps(psp_logmsg))
                else:
                    logger.debug("Message: %r", r)
        await curio.sleep(repeat_delay)

        
async def discover_sensors():
    obj = {'Virtue-protocol-version': VIRTUE_PROTO_VERSION,
           'request': [ uuid.uuid4().hex, 'discovery', '*' ]}

    await send_json_obj(obj)

    response = await recv_json_obj()
    logger.debug(response)

    msg = json.dumps(response, sort_keys=True)

    assert response['reply'][1] == 'discovery', "Unexpected response type"
    sensors = list()
    for s in response['reply'][2]:
        sensors.append(dict(name = s[0], uuid = s[1]))
    return sensors


async def common_init():
    global conn, sensors

    logger.info("Connecting to UNIX socket %s", UNIX_SOCK_PATH)
    conn = await curio.open_unix_connection(UNIX_SOCK_PATH)

    sensors = await discover_sensors()


if __name__ == '__main__':
    logger.addHandler(logging.StreamHandler(sys.stdout))

    if verbose:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    curio.run(common_init)

    if standalone:
        curio.run(poll_sensors)
    else:
        wrapper = sensor_wrapper.SensorWrapper( "linux_kernel", [poll_sensors,] )
        wrapper.start()
