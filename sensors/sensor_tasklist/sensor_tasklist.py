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

    tasklist_canonical_path = which_file("tasklist")

    print("    $ canonical path = %s" % (tasklist_canonical_path,))

    while True:

        # let's profile our ps command
        tasklist_profile = await report_on_file(tasklist_canonical_path)
        psp_logmsg = {
            "timestamp": datetime.datetime.now().isoformat(),
            "level": "info",
            "message": tasklist_profile
        }
        psp_logmsg.update(message_stub)

        await message_queue.put(json.dumps(tasklist_logmsg))

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
    tasklist_path = which_file("tasklist.exe")
    tasklist_args = [" -FO table -NH"]
    self_pid = os.getpid()

    print(" ::starting tasklist %s (pid=%d)" % (tasklist_path, self_pid,))

    full_ps_command = [tasklist_path] + tasklist_args

    while True:

        # run the command and get our output
        p = subprocess.Popen(full_ps_command, stdout=subprocess.PIPE)

        async for line in p.stdout:

            # pull apart our line into something interesting. We want the
            # user, PID, and command for now. Our basic PS line looks like:
            #
            #       root         1  0.0  0.0  21748  1808 pts/0    Ss+  19:54   0:00 /bin/bash /usr/src/app/run.sh

            # we'll split everything apart and drop out whitespace
            line_bits = line.decode("utf-8").split(" ")
            line_bits = [bit for bit in line_bits if bit != '']

            # now let's pull apart the interesting fields
            proc_image_name = line_bits[0]
            proc_pid = line_bits[1]
            proc_session_name = line_bits[2]
            proc_session_number = line_bits[3]
            proc_memory_usage = line_bits[4]

            # don't report on ourself
            if int(proc_pid) == self_pid:
                continue

            # report
            logmsg = {
                "timestamp": datetime.datetime.now().isoformat(),
                "level": "info",
                "message": {
                    "ImageName": proc_image_name,
                    "PID": proc_pid,
                    "SessionName": proc_session_name,
                    "SessionNumber": proc_session_number,
                    "MemUsage": proc_memory_usage
                }
            }
            logmsg.update(message_stub)

            await message_queue.put(json.dumps(logmsg))

        # sleep
        await sleep(repeat_delay)


if __name__ == "__main__":

    wrapper = SensorWrapper("tasklist", [tasklist, assess_tasklist])
    wrapper.start()
