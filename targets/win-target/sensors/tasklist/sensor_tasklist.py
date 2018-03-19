#!"c:\program files\python\python36\python.exe"
import datetime
import json
import re
from curio import subprocess, sleep

from sensor_wrapper import SensorWrapper, report_on_file, which_file


"""
Sensor that wraps the system tasklist command.
"""


async def assess_tasklist(message_stub, config, message_queue):
    """

    :param message_stub:
    :param config:
    :param message_queue:
    :return:
    """
    repeat_delay = config.get("repeat-interval", 15) * 4
    print(" ::starting tasklist check-summing")
    print("    $ repeat-interval = %d" % (repeat_delay,))

    ps_canonical_path = which_file("tasklist")

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


async def tasklist(message_stub, config, message_queue):
    """
    Run the tasklist command and push messages to the output queue.

    :param message_stub: Fields that we need to interpolate into every message
    :param config: Configuration from our sensor, from the Sensing API
    :param message_queue: Shared Queue for messages
    :return: None
    """
    repeat_delay = config.get("repeat-interval", 15)

    print(" ::starting tasklist")
    print("    $ repeat-interval = %d" % (repeat_delay,))

    # map the LSOF types to log levels
    tasklist_type_map = {
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
    p = subprocess.Popen(["tasklist", "-r", "%d" % (repeat_delay,)], stdout=subprocess.PIPE)
    async for line in p.stdout:

        # slice out the TYPE of the tasklist message
        line_bits = re.sub(r'\s+', ' ', line.decode("utf-8")).split(" ")

        if len(line_bits) < 4:
            continue

        line_type = re.sub(r'\s+', ' ', line.decode("utf-8")).split(" ")[4]

        log_level = "debug"
        if line_type in tasklist_type_map:
            log_level = tasklist_type_map[line_type]

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

    wrapper = SensorWrapper("tasklist", [tasklist, assess_tasklist])
    wrapper.start()