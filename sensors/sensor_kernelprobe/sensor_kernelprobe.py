#!"c:\program files\python\python36\python.exe"
"""
Sensor that wraps the system kernelprobe command.
"""
import sys
import json
import logging
import datetime
import configparser

from curio import sleep, UniversalEvent
from sensor_wrapper import SensorWrapper, report_on_file, which_file

__VERSION__ = "1.20180502"
__MODULE__ = "sensor_kernelprobe"

class sensor_kernelprobe(object):
    '''
    kernel probe comms sensor
    '''
    def __init__(self, cfgfilename):
        '''
        construct an instance of this class
        '''
        self._cfgfilename = cfgfilename
        self._config = configparser.ConfigParser()
        self._logger = logging.getLogger(__MODULE__)
        self._logger.setLevel(logging.ERROR)             
        self._logger.info("About to construct the SensorWrapper . . . ")               
        self._wrapper = SensorWrapper("kernelprobe", [self.kernelprobe, self.assess_kernelprobe], 
                                      stop_notification=self.wait_for_service_stop)
        self._logger.info("SensorWrapper constructed . . . ")        
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
            self._logger.critical("Sensor configuration file does not contain"
                                  " a 'parameters' section - exiting!")
            sys.exit(1)
        params = self._config['parameters']
        args = []
        for key in params:
            args.append("--{0}={1}".format(key,params[key],))            
        
        runtime = self._config['runtime']                        
        dbglvl = getattr(logging, runtime['dbglvl'], logging.ERROR)
        self._logger.setLevel(dbglvl)
        
        self._logger.info("Starting the sensor_kernelprobe sensor with parameters %s . . .", args)
        self._running = True
        self._wrapper.start(args)        
    
    def stop(self):
        '''
        stop the sensor
        '''
        self._logger.info("Stopping the sensor_kernelprobe sensor . . . ")        
        self._running = False  # set the tasks to exit
        self._event.set()  # cause the wait_for_service_stop to return
        self._logger.info("Service sensor_kernelprobe sensor has Stopped . . . ")
    
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
        self._logger.info("    $ repeat-interval = %d", repeat_delay)
    
        kernelprobe_canonical_path = which_file("WinVirtUE.sys")
    
        self._logger.info("    $ canonical path = %s", kernelprobe_canonical_path)
    
        while self._running is True:
    
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
        self._logger.info("    $ repeat-interval = %d", repeat_delay)
      
        while self._running is True:    
            # report
            kernelprobe_logmsg = {
                "timestamp": datetime.datetime.now().isoformat(),
                "level": "info",
                "message": {
                    "Test": 123
                }
            }
            kernelprobe_logmsg.update(message_stub)            
            self._logger.debug(json.dumps(kernelprobe_logmsg, indent=3))        
            await message_queue.put(json.dumps(kernelprobe_logmsg))
        
            # sleep
            await sleep(repeat_delay)
        
        self._logger.info("Sensor kernelprobe stopped . . . ")        
