#!"c:\program files\python\python36\python.exe"
'''
Sensor that wraps the sysinternals handle command.
'''
import os
import sys
import json
import logging
import datetime
import subprocess
import configparser

from curio import sleep, UniversalEvent
from sensor_wrapper import SensorWrapper, report_on_file, which_file

__VERSION__ = "1.20180411"
__MODULE__ = "sensor_handlelist"

class sensor_handlelist(object):
    '''
    handle list sensor
    '''    
    def __init__(self, cfgfilename):
        '''
        construct an object instance
        '''
        self._cfgfilename = cfgfilename
        self._config = configparser.ConfigParser()
        self._logger = logging.getLogger(__MODULE__)
        self._logger.setLevel(logging.ERROR)             
        self._logger.info("About to construct the SensorWrapper . . . ")               
        self._wrapper = SensorWrapper("handlelist", [self.handlelist, self.assess_handlelist], 
                                      stop_notification=self.wait_for_service_stop)
        self._logger.info("SensorWrapper constructed. . . ")        
        self._running = False     
        self._event = UniversalEvent()
        
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
            
        self._logger.info("Starting the sensor_handlelist sensor with parameters %s . . .", args)
        self._running = True
        self._wrapper.start(args)        
    
    def stop(self):
        '''
        stop the sensor
        '''
        self._logger.info("Stopping the sensor_handlelist sensor . . . ")
        self._running = False
        self._event.set()  # cause the wait_for_service_stop to return
        self._logger.info("Service sensor_handlelist sensor has Stopped . . . ")        
    
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

    async def wait_for_service_stop(self):
        '''
        Wait for the service stop event.  Do not return until stop notification 
        is received.  As long as this function is active, the SensorWrapper will
        not exit.
        '''
        self._logger.info("Waiting for Service Stop Notification")
        await self._event.wait()
        self._logger.info("Service Stop Recieved - Exiting!")   
    
    async def assess_handlelist(self, message_stub, config, message_queue):
        """
        Assses the handle.exe program
        :param message_stub:
        :param config:
        :param message_queue:
        :return:
        """
        repeat_delay = config.get("repeat-interval", 20) 
        self._logger.info(" ::starting handlelist")
        self._logger.info("    $ repeat-interval = %d", repeat_delay)
    
        handlelist_canonical_path = which_file("handle.exe")
    
        self._logger.info("    $ canonical path = %s", handlelist_canonical_path)
    
        while self.running is True:    
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
    

    async def handlelist(self, message_stub, config, message_queue):
        """
        Run the handle.exe command and push messages to the output queue.
        :param message_stub: Fields that we need to interpolate into every message
        :param config: Configuration from our sensor, from the Sensing API
        :param message_queue: Shared Queue for messages
        :return: None
        """
        repeat_delay = config.get("repeat-interval", 15)
    
        self._logger.info(" ::starting handle.exe")
        self._logger.info("    $ repeat-interval = %d", repeat_delay)
        handlelist_path = which_file("handle.exe")
        handlelist_args = ["-nobanner"]
        self_pid = os.getpid()
    
        self._logger.info(" ::starting handlelist %s (pid=%d)", handlelist_path, self_pid)
    
        full_handlelist_command = [handlelist_path] + handlelist_args
            
        while self.running is True:
            proc_dict = {}
            parse_proc_info_next = False
            first_time_through = True
            # run the command and get our output
            p = subprocess.Popen(full_handlelist_command, stdout=subprocess.PIPE)
            for line in p.stdout:        
                if not line:
                    break            
                line = line.decode('utf-8').strip('\n\r')
                if parse_proc_info_next is True:
                    parse_proc_info_next = False
                    parsed_line = line.split()
                    image_name = parsed_line[0]
                    pid = parsed_line[2]
                    user = ''
                    for ndx in range(3, len(parsed_line)):
                        user += parsed_line[ndx] + ' '
                    proc_dict[pid] = {"PID": pid, "ImageName": image_name, "User": 
                                      user, "Handles": {}}
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
                        self._logger.debug(json.dumps(handlelist_logmsg))
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
    
            self._logger.debug("Sleeping for %d seconds", repeat_delay)
            
            # sleep
            await sleep(repeat_delay)
            