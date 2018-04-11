#!"c:\program files\python\python36\python.exe"
'''
Sensor that wraps the sysinternals handle command.
'''
import os
import json
import logging
import datetime
import subprocess

from curio import sleep
from sensor_wrapper import SensorWrapper, report_on_file, which_file

__VERSION__ = "1.20180411"
__MODULE__ = "sensor_handlelist"

# set up prefix formatting
logfmt='%(asctime)s:%(name)s:%(levelname)s:%(message)s'
logging.basicConfig(format=logfmt,filename="handlelist.log", filemode="w")
# create logger
logger = logging.getLogger(__MODULE__)
logger.setLevel(logging.CRITICAL)

async def assess_handlelist(message_stub, config, message_queue):
    """
    Assses the handle.exe program
    :param message_stub:
    :param config:
    :param message_queue:
    :return:
    """
    repeat_delay = config.get("repeat-interval", 20) 
    logger.info(" ::starting handlelist")
    logger.info("    $ repeat-interval = %d" % (repeat_delay,))

    handlelist_canonical_path = which_file("handle.exe")

    logger.info("    $ canonical path = %s" % (handlelist_canonical_path,))

    while True:

        # let's profile our ps command
        handlelist_profile = await report_on_file(handlelist_canonical_path)
        handlelist_logmsg = {
            "timestamp": datetime.datetime.now().isoformat(),
            "level": "info",
            "message": handlelist_profile
        }
        handlelist_logmsg.update(message_stub)

        await message_queue.put(json.dumps(handlelist_logmsg))

        # sleep
        await sleep(repeat_delay)


async def handlelist(message_stub, config, message_queue):
    """
    Run the handle.exe command and push messages to the output queue.
    :param message_stub: Fields that we need to interpolate into every message
    :param config: Configuration from our sensor, from the Sensing API
    :param message_queue: Shared Queue for messages
    :return: None
    """
    repeat_delay = config.get("repeat-interval", 15)

    logger.info(" ::starting handle.exe")
    logger.info("    $ repeat-interval = %d" % (repeat_delay,))
    handlelist_path = which_file("handle.exe")
    handlelist_args = ["-nobanner"]
    self_pid = os.getpid()

    logger.info(" ::starting handlelist %s (pid=%d)" % (handlelist_path, self_pid,))

    full_handlelist_command = [handlelist_path] + handlelist_args
        
    while True:
        proc_dict = {}
        parse_proc_info_next = False
        first_time_through = True
        # run the command and get our output
        p = subprocess.Popen(full_handlelist_command, stdout=subprocess.PIPE)
        for line in p.stdout:        
            if not line:
                break            
            line = line.decode('utf-8').strip('\n\r')
            if True == parse_proc_info_next:
                parse_proc_info_next = False
                parsed_line = line.split()
                image_name = parsed_line[0]
                pid = parsed_line[2]
                user = ''
                for ndx in range(3, len(parsed_line)):
                    user += parsed_line[ndx] + ' '
                proc_dict[pid]={"PID": pid, "ImageName": image_name, "User": user, "Handles": {}}
                continue
            if line[0:4] == '----':                
                if not first_time_through:
                    
                    # report
                    handlelist_logmsg = {
                        "timestamp": datetime.datetime.now().isoformat(),
                        "level": "info",
                        "message": proc_dict
                    }            
                    handlelist_logmsg.update(message_stub)
                    logger.debug(json.dumps(handlelist_logmsg))
                    await message_queue.put(json.dumps(handlelist_logmsg))
                    proc_dict.clear()
                    proc_dict = {}
                first_time_through = False                                
                parse_proc_info_next = True                
                continue
            
            obj = line.split()
            handle = obj[0].split(':')[0]
            handle_type = obj[1]
            unknown = obj[2:]
            if len(unknown) == 2 and unknown[0][0] == '(' and unknown[0][-1] == ')':
                obj_access = unknown[0]
                obj_path = unknown[1]
            else:
                obj_access = '(---)'
                obj_path = unknown[0]

            entry_dict = {"Handle": handle, "HandleType": handle_type, 
                          "ObjectAccess": obj_access, "ObjectPath": obj_path}

            proc_dict[pid]["Handles"][handle] = entry_dict

        logger.debug("Sleeping for {0} seconds\n".format(repeat_delay,))
        
        # sleep
        await sleep(repeat_delay)

if __name__ == "__main__":
    logger.info("Creating the Sensor Wrapper . . .")
    wrapper = SensorWrapper("handlelist", [handlelist, assess_handlelist])
    logger.info("Starting the Sensor Wrapper . . .")
    wrapper.start()
