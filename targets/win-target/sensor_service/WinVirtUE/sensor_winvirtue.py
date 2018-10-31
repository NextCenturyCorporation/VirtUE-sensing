#!"c:\program files\python\python36\python.exe"
'''
Sensor that monitors threadcreate and thread information
'''
import os
import json
import logging
import datetime
import configparser
from threading import Barrier, Thread
from curio import UniversalEvent, Queue, abide, run
from sensor_wrapper import SensorWrapper

from ntfltmgr import FilterConnectCommunicationPort, EnumerateSensors
from ntfltmgr import  CommandPort, CloseHandle, packet_decode

import __main__ 

__VERSION__ = "1.20180801"
__MODULE__ = "sensor_winvirtue.py"

logger = logging.getLogger(__name__)
logger.addHandler(logging.NullHandler())

class sensor_winvirtue(object):
    '''
    general winvirtue sensor code
    '''    
    BARTIMEOUT = 15
    def __init__(self):
        '''
        construct an object instance
        '''
        self._running = False
        self._sensordict = {}
        self._wrapperdict = {}        
        self._sensorqueues = {}
        self._sensorthddict = {}
        self._defdict = dict(__main__.cfgparser.items('DEFAULT'))
        self._update_sensor_info()
        self._event = UniversalEvent()
        self._barrier = Barrier(len(self._sensordict) + 1, 
                                action=None, 
                                timeout=sensor_winvirtue.BARTIMEOUT)
        self._datastreamthd = Thread(None, self._event_stream_runner, "Data Stream Thread")        
        
    def start(self):
        '''
        start the sensor
        '''         
        self._build_sensor_wrappers()        
        self._running = True
        self._datastreamthd.start() 
        self._start_sensors()
    
    def stop(self):
        '''
        stop the sensor
        '''
        logger.info("Stopping the sensor_winvirtue sensors . . . ")
        for sensor_id in self._wrapperdict:
            sensor_name = self._wrapperdict[sensor_id].sensor_name        
            logger.info("Notifying the %s sensor to stop retrying (if not running) . . .", sensor_name)
            self._wrapperdict[sensor_id].stop_retrying = True                                                 
        self._running = False  # set the tasks to exit
        self._event.set()  # cause the wait_for_service_stop to return
        logger.info("Service sensor_winvirtue sensor has Stopped . . . ")
    
    async def _event_stream_pump(self):
        '''
        The actual async method that is reading from the event stream enqueue the data
        '''
        logger.info("Entering Event Stream Pump!")
        for evt in packet_decode():
            if not self.running:
                logging.info("Terminating packet decode - returning on False running!")
                break
            if not "sensor_id" in evt:
                logger.warning("Streamed Event %s from the driver didn't contain a sensor_id!", evt)
                continue  # if there is no sensor id in this message, log and continue
            sensor_id = evt["sensor_id"]
            await self._sensorqueues[sensor_id].put(evt)
        logging.info("Exiting Event Stream Pump!")
    
    def _event_stream_runner(self):
        '''
        thread entry point for the data stream runner
        @note this thread reads from the event port, converts
        the data stream into a python dictionary and places it on the correct queue
        '''
        run(self._event_stream_pump)    
    
    def _update_sensor_info(self):
        '''
        update sensor information
        '''
        hFltComms = None
        logger.info("Updating sensor information . . . ")        
        try:        
            hFltComms = FilterConnectCommunicationPort(CommandPort)
            logger.info("Connected to Filter Communcations Port %s", CommandPort)
            for sensor in EnumerateSensors(hFltComms):
                if not sensor.Enabled:
                    logger.warn("Sensor %s not enabled - skipping!", 
                            sensor.SensorName)
                    continue
                self._sensordict[sensor.SensorName] = sensor
                logger.info("Discovered Sensor %s",sensor)
        except OSError as osr:
            logger.exception("Cannot update sensor info - check install state %r- fatal!", osr)
            raise
        else:
            logger.info("Successfully updated sensor information")        
        finally:
            CloseHandle(hFltComms)
    
    def _build_sensor_wrappers(self):
        '''
        construct a dictionary of sensor name keyed sensor wrappers
        '''
        logger.info("About to construct the SensorWrapper . . . ")
        for sensorname in self._sensordict:
            sensor_name = sensorname.lower()
            sensor_id = str(self._sensordict[sensorname].sensor_id)
            self._wrapperdict[sensor_id] = SensorWrapper(sensor_name,
                                                       [self.evtdata_consumer],
                                                       stop_notification=self.wait_for_service_stop)
            self._sensorqueues[sensor_id] = Queue()
            logger.info("SensorWrapper for %s id %s constructed . . . ", sensorname, sensor_id)
        logger.info("All SensorWrapper Instances Built . . . ")
                    
    def _load_config_data(self, wrappername):
        '''
        load the sensor specific configuration
        @param wrappername the wrappers name (sensor)
        @returns a dictionary of key/value pairs
        '''
        basedir =  os.path.abspath(os.path.dirname(__file__))
        basedir += os.sep
        cfgfilename = os.path.join(basedir, "config", wrappername + '.cfg')
        logger.info("Loading config data from %s for %s", 
                cfgfilename, wrappername)
        config = configparser.ConfigParser(self._defdict)
        config.read_file(open(cfgfilename))
        logger.info("succesfully loaded data from %s", cfgfilename)
    
        if not config.has_section('parameters'):               
            raise EnvironmentError("Missing sensor configuration file - exiting!")
               
        params = {}
        for key in config['parameters']:
            params[key] = config['parameters'][key]        
        return params
    
    def _start_sensors(self):
        '''
        Start the sensors
        '''
        logger.info("Attempting to start sensors . . .")
        for sensor_id in self._wrapperdict:
            sensor_name = self._wrapperdict[sensor_id].sensor_name
            logger.info("Retrieved sensor id %s from sensor %s", 
                    sensor_id, sensor_name)
            paramdict = self._load_config_data(sensor_name)  # load the configuration data
            logger.info("loaded config data for sensor %s", sensor_id)
            paramdict["sensor_id"] = sensor_id  # artificially inject the sensor id
            logger.info("About to start the %s sensor . . .", sensor_name)
            self._sensorthddict[sensor_id] = Thread(None, self._wrapperdict[sensor_id].start, 
                                     "Sensor %s Start Thread" % (sensor_name,),
                                     args=(), kwargs=paramdict)
            self._sensorthddict[sensor_id].start()
                    
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
        logger.info("Waiting for Service Stop Notification")
        await self._event.wait()
        async with abide(self._barrier):
            await abide(self._barrier.wait)
        logger.info("Service Stop Received - Exiting!")
        
    async def evtdata_consumer(self, message_stub, config, message_queue):
        '''        
        Push messages to the output queue as retrieved from the sensor
        :param message_stub: Fields that we need to interpolate into every message
        :param config: Configuration from our sensor, from the Sensing API
        :param message_queue: Shared Queue for messages
        :return: None
        '''        
        logger.debug("evtdata_consumer(): self=[%r], message_stub=[%r], config=[%r], message_queue=[%r]", 
                    self, message_stub, config, message_queue)
        sensor_id = message_stub["sensor_id"]
        sensor_name = message_stub["sensor_name"]
        sensor_config_level = config.get("sensor-config-level", "default")    
        logger.info(" ::starting %s evtdata_consumer", sensor_name)    
        logger.info("    $ sensor-config-level = %s", sensor_config_level)
    
        queue = self._sensorqueues[sensor_id]
    
        logger.info("Utilizing queue [%r] for sensor id %s", queue, sensor_id)
        dumped = None 
        async for msg in queue:
            if not self.running:
                logger.info("Exiting event data consumer as running is false!")
                break
            event_logmsg = {
                    "timestamp": datetime.datetime.now().isoformat(),
                    "level": "info",
                    "message": msg
                }
            event_logmsg.update(message_stub)
            try:
                dumped = json.dumps(event_logmsg)
                await message_queue.put(dumped)
            except UnicodeEncodeError as ueer:
                logger.exception("Error Decoding Inbound Message [%r]",
                                     ueer, exc_info=True)
                error_logmsg = {
                        "timestamp": datetime.datetime.now().isoformat(),
                        "level": "error",
                        "message": str(ueer)
                    }
                error_logmsg.update(message_stub)
                dumped = json.dumps(error_logmsg)
                await message_queue.put(dumped)
            finally:
                await queue.task_done()
        logger.info("  ::%s evtdata_consumer exiting!", sensor_name)
        
