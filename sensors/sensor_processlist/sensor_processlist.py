#!"c:\program files\python\python36\python.exe"
'''
Sensor that monitors process and thread information
'''
import os
import sys
import json
import logging
import datetime
import subprocess

from curio import sleep
from sensor_wrapper import SensorWrapper
from win32security import LookupPrivilegeValue
from ntsecuritycon import SE_SECURITY_NAME, SE_CREATE_PERMANENT_NAME, SE_DEBUG_NAME
from win32con import SE_PRIVILEGE_ENABLED
from ntquerysys import acquire_privileges, get_process_objects, get_thread_objects

__VERSION__ = "1.20180411"
__MODULE__ = "sensor_processlist"

# set up prefix formatting
logfmt='%(asctime)s:%(name)s:%(levelname)s:%(message)s'
logging.basicConfig(format=logfmt,filename="processlist.log", filemode="w", level=logging.CRITICAL)
# create logger
logger = logging.getLogger(__MODULE__)
logger.setLevel(logging.CRITICAL)

async def process_monitor(message_stub, config, message_queue):
    """
    poll the operating system for process and thread data

    :param message_stub: Fields that we need to interpolate into every message
    :param config: Configuration from our sensor, from the Sensing API
    :param message_queue: Shared Queue for messages
    :return: None
    """
    repeat_delay = config.get("repeat-interval", 15)
    sensor_config_level = config.get("sensor-config-level", "default")
    
    logger.info(" ::starting process monitor")    
    logger.info("    $ repeat-interval = %d" % (repeat_delay,))
    logger.info("    $ sensor-config-level = %s" % (sensor_config_level,))
    
    while True: 
        processlist_logmsg = {}
        proc_dict = {}
        try:                
            for proc_obj, thd_obj in get_process_objects():
                pid = proc_obj["UniqueProcessId"]
                # if the idle process or the process no longer exists
                if not pid:
                    continue                                    
                proc_dict[pid] = proc_obj                
                if sensor_config_level == "adversarial":
                    thd_dict = {}
                    number_of_threads = proc_obj["NumberOfThreads"]
                    for thd in get_thread_objects(number_of_threads, thd_obj):
                        thd_id = thd["UniqueThread"]
                        thd_dict[thd_id] = thd
                    proc_dict[pid]["Threads"] = thd_dict
                    
        except Exception as exc:
            logger.error("Caught Exception {0}\n".format(exc,))
            processlist_logmsg = {
                "timestamp": datetime.datetime.now().isoformat(),
                "level": "error",
                "message": str(exc)
            }                                                        
        else:               
            processlist_logmsg = {
                "timestamp": datetime.datetime.now().isoformat(),
                "level": "info",
                "message": proc_dict
            }                                                
        finally:
            processlist_logmsg.update(message_stub)                                    
            logger.debug(json.dumps(processlist_logmsg, indent=3))        
            await message_queue.put(json.dumps(processlist_logmsg))             
            processlist_logmsg.clear()
            proc_dict.clear()            
                     
        logger.debug("Sleeping for {0} seconds\n".format(repeat_delay,))

        # sleep
        await sleep(repeat_delay)

if __name__ == "__main__":
    new_privs = (
        (LookupPrivilegeValue(None, SE_SECURITY_NAME), SE_PRIVILEGE_ENABLED),
        (LookupPrivilegeValue(None, SE_CREATE_PERMANENT_NAME), SE_PRIVILEGE_ENABLED), 
        (LookupPrivilegeValue(None, SE_DEBUG_NAME), SE_PRIVILEGE_ENABLED)         
    )  

    logger.info("Acquiring Privileges . . .")
    success = acquire_privileges(new_privs)
    if not success:
        logger.critical("Failed to acquire privs!\n")
        sys.exit(-1)   

    logger.info("Creating the Sensor Wrapper . . .")
    wrapper = SensorWrapper("processlist", [process_monitor])
    logger.info("Starting the Sensor Wrapper . . .")
    wrapper.start()
