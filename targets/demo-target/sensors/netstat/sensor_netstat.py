#!/usr/bin/python
import datetime
import json
from curio import subprocess, sleep
import os

from sensor_wrapper import SensorWrapper, which_file, report_on_file

"""
Sensor that wraps the netstat
"""


async def assess_netstat(message_stub, config, message_queue):
    """

    :param message_stub:
    :param config:
    :param message_queue:
    :return:
    """
    repeat_delay = config.get("repeat-interval", 15) * 4
    print(" ::starting netstat check-summing")
    print("    $ repeat-interval = %d" % (repeat_delay,))

    ps_canonical_path = which_file("netstat")

    print("    $ canonical path = %s" % (ps_canonical_path,))

    while True:

        # let's profile our ps command
        ps_profile = await report_on_file(ps_canonical_path)
        psp_logmsg = {
            "timestamp": datetime.datetime.now().isoformat(),
            "level": "info",
            "message": ps_profile
        }
        psp_logmsg.update(message_stub)

        await message_queue.put(json.dumps(psp_logmsg))

        # sleep
        await sleep(repeat_delay)


async def netstat(message_stub, config, message_queue):
    """
    Run the netstat command and push messages to the output queue.

    :param message_stub: Fields that we need to interpolate into every message
    :param config: Configuration from our sensor, from the Sensing API
    :param message_queue: Shared Queue for messages
    :return: None
    """
    repeat_delay = config.get("repeat-interval", 15)
    ps_path = "netstat"
    ps_args = ["-natp"]

    print(" ::starting netstat (%s)" % (ps_path,))
    print("    $ repeat-interval = %d" % (repeat_delay,))

    ps_canonical_path = which_file(ps_path)
    print("    $ canonical path = %s" % (ps_canonical_path,))

    full_ps_command = [ps_path] + ps_args

    while True:

        # run the command and get our output
        p = subprocess.Popen(full_ps_command, stdout=subprocess.PIPE)
        header_count = 0

        async for line in p.stdout:

            # skip the first two lines
            if header_count < 2:
                # don't need this line
                header_count += 1
                continue

            # pull apart our line into something interesting. The basic
            # netstat -natp line looks like:
            #
            #		Active Internet connections (servers and established)
            #		Proto Recv-Q Send-Q Local Address           Foreign Address         State       PID/Program name
            #		tcp        0      0 192.168.1.92:11001      0.0.0.0:*               LISTEN      12/python
            #		tcp        0      0 192.168.1.92:11002      0.0.0.0:*               LISTEN      16/python
            #		tcp        0      0 192.168.1.92:11003      0.0.0.0:*               LISTEN      15/python
            #		tcp        0      0 127.0.0.11:40767        0.0.0.0:*               LISTEN      -
            #		tcp        0      0 192.168.1.92:45456      192.168.1.82:9455       ESTABLISHED 12/python
            #		tcp        0      0 192.168.1.92:43786      192.168.1.86:17504      TIME_WAIT   -
            #		tcp        0      0 192.168.1.92:45496      192.168.1.82:9455       ESTABLISHED 16/python
            #
            # We're going to pull this apart into component fields of:
            #
            #   proto
            #   recv-q
            #   send-q
            #   local (ip / port)
            #   remote (ip / port)
            #   state
            #   pid
            #   program

            # we'll split everything apart and drop out whitespace
            line_bits = line.decode("utf-8").strip().split(" ")
            line_bits = [bit for bit in line_bits if bit != '']

            # Our bits look like:
            #   ['tcp', '0', '0', '192.168.1.92:11001', '0.0.0.0:*', 'LISTEN', '12/python']

            # local address parsing - addr could be ipv4 or ipv6, so don't parse it weird
            local_bits = line_bits[3].split(":")
            l_addr = ":".join(local_bits[:-1])
            l_port = local_bits[-1]

            # remote address parsing - see above for local address
            remote_bits = line_bits[4].split(":")
            r_addr = ":".join(remote_bits[:-1])
            r_port = remote_bits[-1]

            # pid/program parsing
            m_pid = ""
            m_prog = ""

            if "/" in line_bits[-1]:
                (m_pid, m_prog) = line_bits[-1].split("/")

            # report on the ps results
            logmsg = {
                "timestamp": datetime.datetime.now().isoformat(),
                "level": "info",
                "message": {
                    "proto": line_bits[0],
                    "recv-q": line_bits[1],
                    "send-q": line_bits[2],
                    "state": line_bits[5],
                    "local": {
                        "address": l_addr,
                        "port": l_port
                    },
                    "remote": {
                        "address": r_addr,
                        "port": r_port
                    },
                    "pid": m_pid,
                    "program": m_prog
                }
            }
            logmsg.update(message_stub)

            await message_queue.put(json.dumps(logmsg))

        # sleep
        await sleep(repeat_delay)


if __name__ == "__main__":
    wrapper = SensorWrapper("netstat", [netstat, assess_netstat])
    wrapper.start()
