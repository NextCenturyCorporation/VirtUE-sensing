#!/usr/bin/python
import datetime
import json
import os
import re
from curio import subprocess

from sensor_wrapper import SensorWrapper

"""
Sensor that wraps the system PS command.
"""


async def lsof(message_stub, config, message_queue):
    """
    Run the lsof command and push messages to the output queue.

    :param message_stub: Fields that we need to interpolate into every message
    :param config: Configuration from our sensor, from the Sensing API
    :param message_queue: Shared Queue for messages
    :return: None
    """
    repeat_delay = config.get("repeat-interval", 15)

    print(" ::starting lsof")
    print("    $ repeat-interval = %d" % (repeat_delay,))

    # map the LSOF types to log levels
    lsof_type_map = {
        "CHR": "debug",
        "DIR": "debug",
        "IPv4": "info",
        "KQUEUE": "info",
        "NPOLICY": "debug",
        "REG": "debug",
        "systm": "warn",
        "unix": "info",
        "vnode:": "warn"
    }

    # just read from the subprocess and append to the log_message queue
    p = subprocess.Popen(["lsof", "-r", "%d" % (repeat_delay,)], stdout=subprocess.PIPE)
    async for line in p.stdout:

        # slice out the TYPE of the lsof message
        line_bits = re.sub(r'\s+', ' ', line.decode("utf-8")).split(" ")

        if len(line_bits) < 4:
            continue

        line_type = re.sub(r'\s+', ' ', line.decode("utf-8")).split(" ")[4]

        log_level = "debug"
        if line_type in lsof_type_map:
            log_level = lsof_type_map[line_type]

        # build our log message
        logmsg = {
            "message": line.decode("utf-8"),
            "timestamp": datetime.datetime.now().isoformat(),
            "level": log_level
        }

        # interpolate everything from our message stub
        logmsg.update(message_stub)

        await message_queue.put(json.dumps(logmsg))

if __name__ == "__main__":

    wrapper = SensorWrapper("lsof-sensor", lsof)
    wrapper.start()