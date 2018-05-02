#!"c:\program files\python\python36\python.exe"
"""
Sensor that wraps the system kernelprobe command.
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

__VERSION__ = "1.20180502"
__MODULE__ = "sensor_kernelprobe"

class sensor_kernelprobe(object):
    
    def __init__(self, cfgfilename):
        '''
        '''
        self._logger = logging.getLogger(__MODULE__)
        self._logger.setLevel(logging.DEBUG)        
        self._config = configparser.ConfigParser()
        with open(cfgfilename) as f:
            self._config.read_file(f)        
        self._logger.info("Starting the sensor_kernelprobe log . . .")
        self._logger.info("About to construct the SensorWrapper . . . ")               
        self._wrapper = SensorWrapper("kernelprobe", [self.kernelprobe, self.assess_kernelprobe])
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
            #"--public-key-path=c:\\opt\\sensors\\kernelprobe\\certs\\rsa_key.pub", 
            #"--private-key-path=c:\\opt\\sensors\\kernelprobe\\certs\\rsa_key", 
            #"--ca-key-path=c:\\opt\\sensors\\kernelprobe\\certs\\", 
            #"--api-host=sensing-api.savior.internal", "--sensor-port=11020" ]         
        self._logger.info("Starting the sensor_kernelprobe sensor with parameters %s . . ." % args)
        self._running = True
        self._wrapper.start(args)        
    
    def stop(self):
        '''
        stop the sensor
        '''
        self._logger.info("Stopping the sensor_kernelprobe sensor . . . ")
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
        
    async def assess_kernelprobe(self, message_stub, config, message_queue):
        """
        assess WinVirtUE.sys
        :param message_stub:
        :param config:
        :param message_queue:
        :return:
        """
        repeat_delay = config.get("repeat-interval", 15) 
        self._logger.info(" ::starting kernelprobe check-summing")
        self._logger.info("    $ repeat-interval = %d" % (repeat_delay,))
    
        kernelprobe_canonical_path = which_file("WinVirtUE.sys")
    
        self._logger.info("    $ canonical path = %s" % (kernelprobe_canonical_path,))
    
        while True == self._running:
    
            # let's profile our ps command
            kernelprobe_profile = await report_on_file(kernelprobe_canonical_path)
            kernelprobe_logmsg = {
                "timestamp": datetime.datetime.now().isoformat(),
                "level": "info",
                "message": kernelprobe_profile
            }
            kernelprobe_logmsg.update(message_stub)
    
            await message_queue.put(json.dumps(kernelprobe_logmsg))
    
            # sleep
            await sleep(repeat_delay)
            
        self._logger.info("Sensor assess_kernelprobe stopped . . . ")
    
    async def kernelprobe(self, message_stub, config, message_queue):
        """
        Run the kernelprobe command and push messages to the output queue.
    
        :param message_stub: Fields that we need to interpolate into every message
        :param config: Configuration from our sensor, from the Sensing API
        :param message_queue: Shared Queue for messages
        :return: None
        """
        repeat_delay = config.get("repeat-interval", 15)
    
        self._logger.info(" ::starting kernelprobe")
        self._logger.info("    $ repeat-interval = %d" % (repeat_delay,))
        kernelprobe_path = which_file("WinVirtUE.sys")
        kernelprobe_args = ["/FO","CSV","/NH"]
        self_pid = os.getpid()
    
        self._logger.info(" ::starting kernelprobe %s (pid=%d)" % (kernelprobe_path, self_pid,))
    
        full_kernelprobe_command = [kernelprobe_path] + kernelprobe_args
    
        while True == self._running:
            # run the command and get our output
            p = subprocess.Popen(full_kernelprobe_command, stdout=subprocess.PIPE)
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
                kernelprobe_logmsg = {
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
                kernelprobe_logmsg.update(message_stub)            
                self._logger.debug(json.dumps(kernelprobe_logmsg, indent=3))        
                await message_queue.put(json.dumps(kernelprobe_logmsg))
    
            # sleep
            await sleep(repeat_delay)
        self._logger.info("Sensor kernelprobe stopped . . . ")        
