#!/usr/bin/env python3

import datetime
import json
import re
import asyncio

#from curio import subprocess, sleep
from sensor_wrapper import SensorWrapper

import nfs_packet_handler as handler
import nfs_state_monitor as state


"""
Sensor that observes NFS traffic
"""

# Some globals are needed due to constraints on recv_pkt() callback signature

message_queue = None
message_stub  = None
network_iface = None


def find_nfs_iface():
    # Smartly discover the VIF that nfs-server unikernel is on. What
    # follows is "not smart", so fix it.
    return 'vif12.0'


async def recv_pkt( pkt ):
    global message_queue, message_stub

    r = handler.pkt_handler_standard( pkt )
    if not r:
        return
    (level, data) = r
    
    logmsg = { "timestamp" : datetime.datetime.now().isoformat(),
               "level"     : level,
               "action"    : data }

    # interpolate everything from our message stub
    logmsg.update( message_stub )

    await message_queue.put( logmsg )


async def nfs3_sniff( message_stub, config, message_queue ):
    """
    Kick off the NFS sniffer (a long-running activity).

    :param message_stub: Fields that we need to interpolate into every message
    :param config: Configuration from our sensor, from the Sensing API
    :param message_queue: Shared Queue for messages
    :return: None
    """

    network_iface = find_nfs_iface()

    print(" ::starting NFS sniffer")
    print("     using network interface {}".format( network_iface ) )

    nfs.init()

    # TODO: implement custom sniff() that cooperates with asyncio
    sniff( iface=network_iface,
           filter=None,
           store=False,
           prn=recv_pkt )


if __name__ == "__main__":
    wrapper = SensorWrapper( "nfs", [nfs3_sniff,] )
    wrapper.start()
