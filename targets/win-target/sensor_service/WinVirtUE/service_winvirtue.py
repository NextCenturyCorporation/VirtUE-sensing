#!"c:\program files\python\python36\python.exe"
"""
The Windows VirtUE Sensor Services
"""
import sys
import socket
import logging
from threading import Thread

import win32serviceutil
import win32event
import servicemanager
import win32service

from .sensor_winvirtue import sensor_winvirtue

__VERSION__ = "1.20180801"
__MODULE__ = "service_winvirtue.py"

logger = logging.getLogger(__name__)
logger.addHandler(logging.NullHandler())

class WinVirtUE(win32serviceutil.ServiceFramework):
    '''
    kernel probe service that retrieves message from the kernel driver and
    then converts it to json and sends it on its way to the api
    '''
    # you can NET START/STOP the service by the following name  
    _svc_name_ = "WinVirtUE"
    # this text shows up as the service name in the Service  
    _svc_display_name_ = "Windows Virtue Service"
    # this text shows up as the description in the SCM  
    _svc_description_ = "Windows Virtue Management Service"
    
    def __init__(self, args):
        '''
        construct an instance of the WinVirtUE service
        '''        
        win32serviceutil.ServiceFramework.__init__(self, args)        
        socket.setdefaulttimeout(60)
        self.hWaitStop = win32event.CreateEvent(None, 0, 0, None)
        servicemanager.LogInfoMsg("WinVirtUE Service Initializing")
        logger.info("WinVirtUE Service  . . . ")
        self._sensor = sensor_winvirtue()
        logger.info("WinVirtUE constructed. . . ")    
        self._svcthd = Thread(None, self._sensor.start, "WinVirtUE thread")
        servicemanager.LogInfoMsg("WinVirtUE Service Initialized")                                             
        
    def SvcStop(self):
        '''
        Stop this service
        '''
        self.ReportServiceStatus(win32service.SERVICE_STOP_PENDING, waitHint = 30000)
        servicemanager.LogInfoMsg("Service WinVirtUE State STOP PENDING")
        logger.info("Stopping the WinVirtUE sensor . . . ")
        self._sensor.stop()
        self._svcthd.join(15.0)  # 15 seconds to shutdown the service cleanly
        self.ReportServiceStatus(win32service.SERVICE_STOP)
        logger.info("WinVirtUE Service has stopped. . . ")
        servicemanager.LogWarningMsg("Service WinVirtUE State STOP")        
        win32event.SetEvent(self.hWaitStop)
                    
    def SvcDoRun(self):
        '''
        Just make sure we are running
        '''
        servicemanager.LogInfoMsg("WinVirtUE SvcDoRun Entered")
        logger.info("Starting the WinVirtUE sensor . . . ")
        servicemanager.LogInfoMsg("WinVirtUE Sensors Started")
        try:    
            self._svcthd.start()
        except EnvironmentError as eerr:
            logger.exception("Error occurred attempting to start: %r", eerr, exc_info=True)
            servicemanager.LogErrorMsg("Error occurred attempting to start: %r", eerr)
            sys.exit(-1)
        else:
            win32event.WaitForSingleObject(self.hWaitStop, win32event.INFINITE)
            logger.info("Returned from Starting the WinVirtUE sensor . . . ")
    
    def SvcShutdown(self):
        '''
        Called when host is shutting down
        '''
        servicemanager.LogWarningMsg("Host is shutting down!")
        logger.warning("Host is shutting down!")        
        self.SvcStop()  
       
