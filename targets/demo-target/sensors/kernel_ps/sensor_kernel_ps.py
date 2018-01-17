#!/usr/bin/python
import datetime
import json
from curio import subprocess, sleep
import os

from sensor_wrapper import SensorWrapper, which_file, report_on_file

"""
Sensor that wraps the system PS command.
"""
async def assess_ps(message_stub, config, message_queue):
    """

    :param message_stub:
    :param config:
    :param message_queue:
    :return:
    """
    repeat_delay = config.get("repeat-interval", 15) * 4
    print(" ::starting kernel-ps check-summing")
    print("    $ repeat-interval = %d" % (repeat_delay,))

    ps_canonical_path = which_file(wrapper.opts.ps_path)

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


async def ps(message_stub, config, message_queue):
    """
    Run the PS command and push messages to the output queue.

    :param message_stub: Fields that we need to interpolate into every message
    :param config: Configuration from our sensor, from the Sensing API
    :param message_queue: Shared Queue for messages
    :return: None
    """
    repeat_delay = config.get("repeat-interval", 15)
    ps_path = wrapper.opts.ps_path
    ps_args = ["-auxxx"]
    self_pid = os.getpid()

    print(" ::starting kernel-ps (%s) (pid=%d)" % (ps_path, self_pid))
    print("    $ repeat-interval = %d" % (repeat_delay,))

    full_ps_command = [ps_path] + ps_args

    while True:

        # run the command and get our output
        p = subprocess.Popen(full_ps_command, stdout=subprocess.PIPE)
        got_header = False

        async for line in p.stdout:

            # skip the first line
            if got_header is False:
                # don't need this line
                got_header = True
                continue

            # pull apart our line into something interesting. We want the
            # user, PID, and command for now. Our basic PS line looks like:
            #
            #       root         1  0.0  0.0  21748  1808 pts/0    Ss+  19:54   0:00 /bin/bash /usr/src/app/run.sh

            # we'll split everything apart and drop out whitespace
            line_bits = line.decode("utf-8").split(" ")
            line_bits = [bit for bit in line_bits if bit != '']

            # now let's pull apart the interesting fields
            proc_user = line_bits[0]
            proc_pid = line_bits[1]
            proc_proc = " ".join(line_bits[10:]).strip()

            # don't report on ourself
            if int(proc_pid) == self_pid:
                continue

            # report
            logmsg = {
                "timestamp": datetime.datetime.now().isoformat(),
                "level": "info",
                "message": {
                    "process": proc_proc,
                    "pid": proc_pid,
                    "user": proc_user
                }
            }
            logmsg.update(message_stub)

            await message_queue.put(json.dumps(logmsg))

        # sleep
        await sleep(repeat_delay)


if __name__ == "__main__":
    wrapper = SensorWrapper("kernel-ps-sensor", [ps, assess_ps])

    wrapper.argparser.add_argument("--ps-path", dest="ps_path", default="/tmp/ps", help="Path to the PS command")
    wrapper.start()