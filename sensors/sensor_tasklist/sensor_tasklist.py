#!"c:\program files\python\python36\python.exe"
"""
Sensor that wraps the system tasklist command.
"""
import os
import json
import logging
import datetime
import subprocess

from curio import sleep
from sensor_wrapper import SensorWrapper, report_on_file, which_file

__VERSION__ = "1.20180404"
__MODULE__ = "sensor_tasklist"

# set up prefix formatting
logfmt='%(asctime)s:%(name)s:%(levelname)s:%(message)s'
logging.basicConfig(format=logfmt,filename="handlelist.log", filemode="w")
# create logger
logger = logging.getLogger(__MODULE__)
logger.setLevel(logging.CRITICAL)

async def assess_tasklist(message_stub, config, message_queue):
    """
    assess task.exe
    :param message_stub:
    :param config:
    :param message_queue:
    :return:
    """
    repeat_delay = config.get("repeat-interval", 15) 
    logger.info(" ::starting tasklist check-summing")
    logger.info("    $ repeat-interval = %d" % (repeat_delay,))

    tasklist_canonical_path = which_file("tasklist.exe")

    logger.info("    $ canonical path = %s" % (tasklist_canonical_path,))

    while True:

        # let's profile our ps command
        tasklist_profile = await report_on_file(tasklist_canonical_path)
        tasklist_logmsg = {
            "timestamp": datetime.datetime.now().isoformat(),
            "level": "info",
            "message": tasklist_profile
        }
        tasklist_logmsg.update(message_stub)

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

    logger.info(" ::starting tasklist")
    logger.info("    $ repeat-interval = %d" % (repeat_delay,))
    tasklist_path = which_file("tasklist.exe")
    tasklist_args = ["/FO","CSV","/NH"]
    self_pid = os.getpid()

    logger.info(" ::starting tasklist %s (pid=%d)" % (tasklist_path, self_pid,))

    full_tasklist_command = [tasklist_path] + tasklist_args

    while True:
        # run the command and get our output
        p = subprocess.Popen(full_tasklist_command, stdout=subprocess.PIPE)
        for line in p.stdout:
            # pull apart our line into something interesting. We want the
            # user, PID, and command for now. Our basic PS line looks like:
            #
            # Image Name   PID Session Name  Session# Mem Usage
            # System Idle Process  0 Services  0          8 K

            # we'll split everything apart and drop out whitespace
            line_bits = line.decode("utf-8").split(",")
            line_bits = [bit for bit in line_bits if bit != '']

            # now let's pull apart the interesting fields
            proc_image_name = line_bits[0]
            proc_pid = line_bits[1].strip('"')
            proc_session_name = line_bits[2]
            proc_session_number = line_bits[3].strip('"')
            proc_memory_usage = line_bits[4]

            if int(proc_pid) == self_pid:
                continue

            # report
            tasklist_logmsg = {
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
            tasklist_logmsg.update(message_stub)            
            logger.debug(json.dumps(tasklist_logmsg, indent=3))        
            await message_queue.put(json.dumps(tasklist_logmsg))

        # sleep
        await sleep(repeat_delay)


if __name__ == "__main__":
    logger.info("Creating the Sensor Wrapper . . .")
    wrapper = SensorWrapper("tasklist", [tasklist, assess_tasklist])
    logger.info("Starting the Sensor Wrapper . . .")
    wrapper.start()
