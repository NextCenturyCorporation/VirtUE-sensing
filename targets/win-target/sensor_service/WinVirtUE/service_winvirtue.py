#!"c:\program files\python\python36\python.exe"
"""
The Windows VirtUE Sensor Services
"""

import logging
import socket
import sys
import os

import win32serviceutil
import servicemanager
import win32service

from sensor_processlist import sensor_processlist
from sensor_handlelist import sensor_handlelist
from sensor_kernelprobe import sensor_kernelprobe

__VERSION__ = "1.20180420"
__MODULE__ = "WinVirtUE"

# set up prefix formatting
logging.basicConfig(        
    level       = logging.ERROR,
    format      = '%(asctime)s:%(name)s:%(levelname)s:%(message)s'
)  

class processlist_service(win32serviceutil.ServiceFramework):
    '''
    processlist service implements the process list service to deliver
    sensor_processlist data to the api
    '''    
    # you can NET START/STOP the service by the following name  
    _svc_name_ = "processlist_service"
    # this text shows up as the service name in the Service  
    _svc_display_name_ = "Process List Service"
    # this text shows up as the description in the SCM  
    _svc_description_ = "Windows Savior Process List Service Sensor"

    def __init__(self, args):
        '''
        construct an instance of this class
        '''
        win32serviceutil.ServiceFramework.__init__(self, args)        
        socket.setdefaulttimeout(60)
        servicemanager.LogInfoMsg("processlist_service Service Initializing")
        self._logger = logging.getLogger(processlist_service._svc_name_)
        logfilename = os.path.join(os.environ["SystemDrive"], os.sep, __MODULE__, 
                                "logs", processlist_service._svc_name_ + '.log')
        file_handler = logging.FileHandler(logfilename)
        self._logger.addHandler(file_handler)
        self._logger.setLevel(logging.ERROR)
        self._logger.info("Constructing the processlist sensor . . . ")
        cfgfilename = os.path.join(os.environ["SystemDrive"], os.sep, __MODULE__, 
                                       "config", processlist_service._svc_name_ + '.cfg')        
        self.sensor = sensor_processlist(cfgfilename)
        self._logger.info("processlist sensor constructed. . . ")    
        servicemanager.LogInfoMsg("processlist_service Service Initialized")             
                                 
    def SvcStop(self):
        '''
        Stop this service
        '''        
        self.ReportServiceStatus(win32service.SERVICE_STOP_PENDING, waitHint = 30000)
        servicemanager.LogInfoMsg("Service processlist_service State STOP PENDING")
        self._logger.info("Stopping the processlist sensor . . . ")
        self.sensor.stop()
        self.ReportServiceStatus(win32service.SERVICE_STOP)
        self._logger.info("Processlist sensor has stopped. . . ")
        servicemanager.LogInfoMsg("Service processlist_service State STOP")        
        
    def SvcDoRun(self):
        '''
        Just make sure we are running
        '''
        servicemanager.LogInfoMsg("Service processlist_service SvcDoRun Entered")
        self._logger.info("Starting the processlist sensor . . . ")
        self.sensor.start()
        self._logger.info("Returned from Starting the processlist sensor . . . ")
    
    def SvcShutdown(self):
        '''
        Called when host is shutting down
        '''
        self._logger.warning("Host is shutting down!")        
        self.SvcStop()      
                       
class handlelist_service(win32serviceutil.ServiceFramework):
    '''
    handlelist service implements the handle list service to deliver
    sensor_handlelist data to the api
    '''
    # you can NET START/STOP the service by the following name  
    _svc_name_ = "handlelist_service"
    # this text shows up as the service name in the Service  
    _svc_display_name_ = "Handle List Service"
    # this text shows up as the description in the SCM  
    _svc_description_ = "Windows Savior Handle List Service Sensor"    
    
    def __init__(self, args):
        win32serviceutil.ServiceFramework.__init__(self, args)        
        socket.setdefaulttimeout(60)
        servicemanager.LogInfoMsg("handlelist_service Service Initializing")
        self._logger = logging.getLogger(handlelist_service._svc_name_)
        logfilename = os.path.join(os.environ["SystemDrive"], os.sep, __MODULE__, 
                                "logs", handlelist_service._svc_name_ + '.log') 
        file_handler = logging.FileHandler(logfilename)
        self._logger.addHandler(file_handler)
        self._logger.setLevel(logging.ERROR)
        self._logger.info("Constructing the handlelist sensor . . . ")
        cfgfilename = os.path.join(os.environ["SystemDrive"], os.sep, __MODULE__, 
                                       "config", handlelist_service._svc_name_ + '.cfg')               
        self.sensor = sensor_handlelist(cfgfilename)
        self._logger.info("handlelist sensor constructed. . . ")    
        servicemanager.LogInfoMsg("handlelist_service Service Initialized")             
                                 
    def SvcStop(self):
        '''
        Stop this service
        '''
        self.ReportServiceStatus(win32service.SERVICE_STOP_PENDING, waitHint = 30000)
        servicemanager.LogInfoMsg("Service handlelist_service State STOP PENDING")
        self._logger.info("Stopping the handlelist sensor . . . ")
        self.sensor.stop()
        self.ReportServiceStatus(win32service.SERVICE_STOP)
        self._logger.info("Handlelist sensor has stopped. . . ")
        servicemanager.LogInfoMsg("Service handlelist_service State STOP")
        
    def SvcDoRun(self):
        '''
        Just make sure we are running
        '''
        servicemanager.LogInfoMsg("Service SvcDoRun Entered")
        self._logger.info("Starting the handlelist sensor . . . ")
        self.sensor.start()
        self._logger.info("Returned from Starting the handlelist sensor . . . ")
    
    def SvcShutdown(self):
        '''
        Called when host is shutting down
        '''
        self._logger.warning("Host is shutting down!")        
        self.SvcStop()  
        
class kernelprobe_service(win32serviceutil.ServiceFramework):
    '''
    kernel probe service that retrieves message from the kernel driver and
    then converts it to json and sends it on its way to the api
    '''
    # you can NET START/STOP the service by the following name  
    _svc_name_ = "kernelprobe_service"
    # this text shows up as the service name in the Service  
    _svc_display_name_ = "Kernel Probe Service"
    # this text shows up as the description in the SCM  
    _svc_description_ = "Windows Savior Kernel Probe Service Sensor"
    
    def __init__(self, args):
        '''
        construct an instance of this service
        '''
        win32serviceutil.ServiceFramework.__init__(self, args)        
        socket.setdefaulttimeout(60)
        servicemanager.LogInfoMsg("kernelprobe_service Service Initializing")
        self._logger = logging.getLogger(kernelprobe_service._svc_name_)
        logfilename = os.path.join(os.environ["SystemDrive"], os.sep, __MODULE__, 
                                "logs", kernelprobe_service._svc_name_ + '.log')
        cfgfilename = os.path.join(os.environ["SystemDrive"], os.sep, __MODULE__, 
                                       "config", kernelprobe_service._svc_name_ + '.cfg')        
        file_handler = logging.FileHandler(logfilename)
        self._logger.addHandler(file_handler)
        self._logger.setLevel(logging.ERROR)        
        self._logger.info("Constructing the kernelprobe sensor . . . ")
        self.sensor = sensor_kernelprobe(cfgfilename)
        self._logger.info("kernelprobe sensor constructed. . . ")    
        servicemanager.LogInfoMsg("kernelprobe_service Service Initialized")                                             
        
    def SvcStop(self):
        '''
        Stop this service
        '''
        self.ReportServiceStatus(win32service.SERVICE_STOP_PENDING, waitHint = 30000)
        servicemanager.LogInfoMsg("Service kernelprobe_service State STOP PENDING")
        self._logger.info("Stopping the kernelprobe sensor . . . ")
        self.sensor.stop()
        self.ReportServiceStatus(win32service.SERVICE_STOP)
        self._logger.info("Kernelprobe sensor has stopped. . . ")
        servicemanager.LogInfoMsg("Service kernelprobe_service State STOP")        
                    
    def SvcDoRun(self):
        '''
        Just make sure we are running
        '''
        servicemanager.LogInfoMsg("Service SvcDoRun Entered")
        self._logger.info("Starting the kernelprobe sensor . . . ")
        self.sensor.start()
        self._logger.info("Returned from Starting the kernelprobe sensor . . . ")
    
    def SvcShutdown(self):
        '''
        Called when host is shutting down
        '''
        self._logger.warning("Host is shutting down!")        
        self.SvcStop()  
        
if __name__ == '__main__':
    if len(sys.argv) == 1:
        servicemanager.Initialize()
        servicemanager.PrepareToHostMultiple(processlist_service._svc_name_, processlist_service)        
        servicemanager.PrepareToHostMultiple(handlelist_service._svc_name_, handlelist_service)        
        servicemanager.PrepareToHostMultiple(kernelprobe_service._svc_name_, kernelprobe_service)  
        servicemanager.StartServiceCtrlDispatcher()
    else:
        sys.frozen = 'windows_exe'
        win32serviceutil.HandleCommandLine(processlist_service)        
        win32serviceutil.HandleCommandLine(handlelist_service)        
        win32serviceutil.HandleCommandLine(kernelprobe_service)     
