#!/usr/bin/python
import datetime
import json
import re
from curio import subprocess, sleep

from sensor_wrapper import SensorWrapper, report_on_file, which_file


"""
Sensor that wraps the system tcpdump command.
"""


async def assess_tcpdump(message_stub, config, message_queue):
    """

    :param message_stub:
    :param config:
    :param message_queue:
    :return:
    """
    repeat_delay = config.get("repeat-interval", 15) * 4
    print(" ::starting tcpdump check-summing")
    print("    $ repeat-interval = %d" % (repeat_delay,))

    ps_canonical_path = which_file("tcpdump")

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


async def tcpdump(message_stub, config, message_queue):
    """
    Run the lsof command and push messages to the output queue.

    :param message_stub: Fields that we need to interpolate into every message
    :param config: Configuration from our sensor, from the Sensing API
    :param message_queue: Shared Queue for messages
    :return: None
    """
    command_args = config.get("args", ["-i", "any"])

    print(" ::starting tcpdump")
    print("    $ args = %s" % (command_args,))

    # just read from the subprocess and append to the log_message queue
    full_args = ["tcpdump"] + command_args

    # streams data forever
    async with subprocess.Popen(full_args, stdout=subprocess.PIPE) as p:
        async for line in p.stdout:
            # TODO: multi-line output from tcpdump not supported

            # build our log message
            logmsg = {
                "message": line.decode("utf-8"),
                "timestamp": datetime.datetime.now().isoformat(),
                "level": "info"
            }

            # interpolate everything from our message stub
            logmsg.update(message_stub)

            await message_queue.put(json.dumps(logmsg))


if __name__ == "__main__":

    wrapper = SensorWrapper("tcpdump", [tcpdump, assess_tcpdump])
    wrapper.start()
