#!"c:\program files\python\python36\python.exe"
import datetime
import json
import re
import os
from curio import sleep
import subprocess
from sensor_wrapper import SensorWrapper, report_on_file, which_file


"""
Sensor that wraps the ntquerysystem process list
"""


async def assess_processlist(message_stub, config, message_queue):
    """

    :param message_stub:
    :param config:
    :param message_queue:
    :return:
    """
    repeat_delay = config.get("repeat-interval", 15) 
    print(" ::starting processlist check-summing")
    print("    $ repeat-interval = %d" % (repeat_delay,))

    while True:
        processlist_logmsg = {
            "timestamp": datetime.datetime.now().isoformat(),
            "level": "info",
            "message": {}   # TODO:  What kind of message should I put here?
        }
        processlist_logmsg.update(message_stub)

        await message_queue.put(json.dumps(processlist_logmsg))

        # sleep
        await sleep(repeat_delay)


import pywintypes
from win32security import LookupPrivilegeValue
from ntsecuritycon import SE_SECURITY_NAME, SE_CREATE_PERMANENT_NAME, SE_DEBUG_NAME
from win32con import SE_PRIVILEGE_ENABLED
from ntquerysys import get_basic_system_information, get_process_ids, get_process_objects, acquire_privileges, release_privileges

async def processlist(message_stub, config, message_queue):
    """
    Run the processlist command and push messages to the output queue.

    :param message_stub: Fields that we need to interpolate into every message
    :param config: Configuration from our sensor, from the Sensing API
    :param message_queue: Shared Queue for messages
    :return: None
    """
    repeat_delay = config.get("repeat-interval", 15)

    print(" ::starting processlist")
    print("    $ repeat-interval = %d" % (repeat_delay,))
    self_pid = os.getpid()

    new_privs = (
        (LookupPrivilegeValue(None, SE_SECURITY_NAME), SE_PRIVILEGE_ENABLED),
        (LookupPrivilegeValue(None, SE_CREATE_PERMANENT_NAME), SE_PRIVILEGE_ENABLED), 
        (LookupPrivilegeValue(None, SE_DEBUG_NAME), SE_PRIVILEGE_ENABLED)         
    )  
    
    success = acquire_privileges(new_privs)
    if not success:
        print("Failed to acquire privs!\n")
        sys.exit(-1)    
        
    print(" ::starting processlist (pid=%d)" % (self_pid,))


    try:        
        sbi = get_basic_system_information()
        print("System Basic Info = {0}\n".format(sbi,))

        while True:
            for pid in get_process_ids():                    
                try:
                    proc_obj = get_process_objects(pid)
                    print("Process ID {0} Handle Information: {1}\n"
                            .format(pid,proc_obj,))                
                except pywintypes.error as err:
                    print("Unable to get handle information from pid {0} - {1}\n"
                            .format(pid, err,))
    finally:
        success = release_privileges(new_privs)


    while True:

        # run the command and get our output
        p = subprocess.Popen(full_processlist_command, stdout=subprocess.PIPE)
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

    wrapper = SensorWrapper("processlist", [processlist, assess_processlist])
    wrapper.start()
