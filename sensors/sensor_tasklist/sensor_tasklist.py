#!"c:\program files\python\python36\python.exe"
"""
Sensor that wraps the system tasklist command.
"""
import os
import sys
import json
import logging
import datetime
import subprocess
import configparser

from curio import sleep
from sensor_wrapper import SensorWrapper, report_on_file, which_file

__VERSION__ = "1.20180404"
__MODULE__ = "sensor_tasklist"

class sensor_tasklist(object):
    
    def __init__(self):
        '''
        '''
        self._logger = logging.getLogger(__MODULE__)
        self._logger.setLevel(logging.DEBUG)        
        import pdb;pdb.set_trace()
        self._config = configparser.ConfigParser()
        with open('c:\\opt\\sensors\\tasklist\\tasklist.cfg') as f:
            self._config.read_file(f)        
        self._logger.info("Starting the sensor_tasklist log . . .")
        self._logger.info("About to construct the SensorWrapper . . . ")               
        self._wrapper = SensorWrapper("tasklist", [self.tasklist, self.assess_tasklist])
        self._logger.info("SensorWrapper constructed. . . ")        
        self._running = False        
        
    def start(self):
        '''
        start the sensor
        ''' 
        if not self._config.has_section('parameters'):
            self._logger.critical("Sensor configuration file does not contain a 'parameters' section - exiting!")
            sys.exit(1)
        params = self._config['parameters']
        args = []
        for key in params:
            args.append("--{0}={1}".format(key,params[key],))
            
        #args = [
            #"--public-key-path=c:\\opt\\sensors\\tasklist\\certs\\rsa_key.pub", 
            #"--private-key-path=c:\\opt\\sensors\\tasklist\\certs\\rsa_key", 
            #"--ca-key-path=c:\\opt\\sensors\\tasklist\\certs\\", 
            #"--api-host=sensing-api.savior.internal", "--sensor-port=11020" ]         
        self._logger.info("Starting the sensor_tasklist sensor with parameters %s . . ." % args)
        self._running = True
        self._wrapper.start(args)        
    
    def stop(self):
        '''
        stop the sensor
        '''
        self._logger.info("Stopping the sensor_tasklist sensor . . . ")
        self._running = False
        
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
        
    async def assess_tasklist(self, message_stub, config, message_queue):
        """
        assess task.exe
        :param message_stub:
        :param config:
        :param message_queue:
        :return:
        """
        repeat_delay = config.get("repeat-interval", 15) 
        self._logger.info(" ::starting tasklist check-summing")
        self._logger.info("    $ repeat-interval = %d" % (repeat_delay,))
    
        tasklist_canonical_path = which_file("tasklist.exe")
    
        self._logger.info("    $ canonical path = %s" % (tasklist_canonical_path,))
    
        while True == self._running:
    
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
            
        self._logger.info("Sensor assess_tasklist stopped . . . ")
    
    async def tasklist(self, message_stub, config, message_queue):
        """
        Run the tasklist command and push messages to the output queue.
    
        :param message_stub: Fields that we need to interpolate into every message
        :param config: Configuration from our sensor, from the Sensing API
        :param message_queue: Shared Queue for messages
        :return: None
        """
        repeat_delay = config.get("repeat-interval", 15)
    
        self._logger.info(" ::starting tasklist")
        self._logger.info("    $ repeat-interval = %d" % (repeat_delay,))
        tasklist_path = which_file("tasklist.exe")
        tasklist_args = ["/FO","CSV","/NH"]
        self_pid = os.getpid()
    
        self._logger.info(" ::starting tasklist %s (pid=%d)" % (tasklist_path, self_pid,))
    
        full_tasklist_command = [tasklist_path] + tasklist_args
    
        while True == self._running:
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
                self._logger.debug(json.dumps(tasklist_logmsg, indent=3))        
                await message_queue.put(json.dumps(tasklist_logmsg))
    
            # sleep
            await sleep(repeat_delay)
        self._logger.info("Sensor tasklist stopped . . . ")        
