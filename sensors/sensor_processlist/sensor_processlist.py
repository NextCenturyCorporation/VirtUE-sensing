#!"c:\program files\python\python36\python.exe"
'''
Sensor that monitors process and thread information
'''
import sys
import json
import logging
import datetime
import configparser

from ntquerysys import acquire_privileges, get_process_objects, get_thread_objects

from ntsecuritycon import SE_SECURITY_NAME, SE_CREATE_PERMANENT_NAME, SE_DEBUG_NAME
from win32con import SE_PRIVILEGE_ENABLED

from curio import sleep
from sensor_wrapper import SensorWrapper
from win32security import LookupPrivilegeValue

__VERSION__ = "1.20180411"
__MODULE__ = "sensor_processlist"

class sensor_processlist(object):
    '''
    handle list sensor
    '''    
    new_privs = (
        (LookupPrivilegeValue(None, SE_SECURITY_NAME), SE_PRIVILEGE_ENABLED),
        (LookupPrivilegeValue(None, SE_CREATE_PERMANENT_NAME), SE_PRIVILEGE_ENABLED), 
        (LookupPrivilegeValue(None, SE_DEBUG_NAME), SE_PRIVILEGE_ENABLED)         
    )      
    
    def __init__(self, cfgfilename):
        '''
        construct an object instance
        '''
        self._cfgfilename = cfgfilename
        self._config = configparser.ConfigParser()
        self._logger = logging.getLogger(__MODULE__)
        self._logger.setLevel(logging.ERROR)               
        self._logger.info("Acquiring Privileges . . .")
        success = acquire_privileges(sensor_processlist.new_privs)
        if not success:
            self._logger.critical("Failed to acquire privs!\n")
            sys.exit(-1)               
                    
        self._logger.info("About to construct the SensorWrapper . . . ")
        self._wrapper = SensorWrapper("processlist", [self.process_monitor])
        self._logger.info("SensorWrapper constructed. . . ")        
        self._running = False        
        
    def start(self):
        '''
        start the sensor
        ''' 
        with open(self._cfgfilename) as f:
            self._config.read_file(f)
            
        if (not self._config.has_section('parameters')
            or not self._config.has_section('runtime')):
            self._logger.critical("Sensor configuration file does not contain "
                                  "a 'parameters' section - exiting!")
            sys.exit(1)
        params = self._config['parameters']
        args = []
        for key in params:
            args.append("--{0}={1}".format(key,params[key],))
        
        runtime = self._config['runtime']                        
        dbglvl = getattr(logging, runtime['dbglvl'], logging.ERROR)
        self._logger.setLevel(dbglvl)
        
        self._logger.info("Starting the sensor_processlist sensor with parameters %s . . .", args)
        self._running = True
        self._wrapper.start(args)        
    
    def stop(self):
        '''
        stop the sensor
        '''
        self._logger.info("Stopping the sensor_processlist sensor . . . ")
        self._running = False
    
    @property
    def config(self):
        '''
        This services configuration information
        '''
        return self._config
    
    @property
    def running(self):
        '''
        returns the current running state
        '''
        return self._running
    
    @running.setter
    def running(self, value):
        '''
        sets the running state
        '''
        self._running = value
        
                
    async def process_monitor(self, message_stub, config, message_queue):
        """
        poll the operating system for process and thread data
    
        :param message_stub: Fields that we need to interpolate into every message
        :param config: Configuration from our sensor, from the Sensing API
        :param message_queue: Shared Queue for messages
        :return: None
        """
        repeat_delay = config.get("repeat-interval", 15)
        sensor_config_level = config.get("sensor-config-level", "default")
        
        self._logger.info(" ::starting process monitor")    
        self._logger.info("    $ repeat-interval = %d", repeat_delay)
        self._logger.info("    $ sensor-config-level = %s", sensor_config_level)
        
        while self.running is True:
            try:                
                for proc_obj, thd_obj in get_process_objects():
                    proc_dict = {}
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
    
                    processlist_logmsg = {
                        "timestamp": datetime.datetime.now().isoformat(),
                        "level": "info",
                        "message": proc_dict
                    }                                                
                    processlist_logmsg.update(message_stub)
                    self._logger.debug(json.dumps(processlist_logmsg, indent=3))        
                    await message_queue.put(json.dumps(processlist_logmsg))             
            except Exception as exc:
                self._logger.exception("Caught Exception %s", exc)
                processlist_logmsg = {
                    "timestamp": datetime.datetime.now().isoformat(),
                    "level": "error",
                    "message": str(exc)
                }
                processlist_logmsg.update(message_stub)                                    
                self._logger.debug(json.dumps(processlist_logmsg, indent=3))        
                await message_queue.put(json.dumps(processlist_logmsg))             
                         
            self._logger.debug("Sleeping for %d seconds", repeat_delay)
    
            # sleep
            await sleep(repeat_delay)
        
