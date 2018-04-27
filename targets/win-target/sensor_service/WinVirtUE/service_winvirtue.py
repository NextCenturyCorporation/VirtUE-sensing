#!"c:\program files\python\python36\python.exe"
"""
The Windows VirtUE Sensor Services
"""

import threading
import signal
import logging
import socket
import sys
import win32serviceutil
import win32service
import servicemanager


from sensor_tasklist import sensor_tasklist

__VERSION__ = "1.20180420"
__MODULE__ = "service_winvirtue"

# set up prefix formatting
logfmt='%(asctime)s:%(name)s:%(levelname)s:%(message)s'

class tasklist_service(win32serviceutil.ServiceFramework):
    _svc_name_ = "tasklist_service"
    _svc_display_name_ = "Tasklist Service"
    _svc_description_ = "Windows Savior Tasklist Service Sensor"
    logging.basicConfig(
        filename    = 'c:\\Temp\\{}.log'.format(_svc_name_),
        level       = logging.DEBUG,
        format      = logfmt
    )  
    
    def __init__(self, args):
        win32serviceutil.ServiceFramework.__init__(self, args)        
        socket.setdefaulttimeout(60)
        servicemanager.LogInfoMsg("tasklist_service Service Initializing")
        self._logger = logging.getLogger(__MODULE__)
        self._logger.setLevel(logging.DEBUG)
        self._logger.info("tasklist_service.__init__(): current thread is {0}".format(threading.current_thread(),))
        self._logger.info("Starting the tasklist_service log . . .")        
        self._logger.info("Constructing the tasklist sensor . . . ")
        self.sensor = sensor_tasklist()
        self._logger.info("tasklist sensor constructed. . . ")    
        servicemanager.LogInfoMsg("tasklist_service Service Initialized")             
                                 
    def SvcStop(self):
        self.ReportServiceStatus(win32service.SERVICE_STOP_PENDING)     
        servicemanager.LogInfoMsg("Service State STOP PENDING")
        self._logger.info("Stopping the tasklist sensor . . . ")
        self.sensor.stop()
        
    def SvcDoRun(self):
        '''
        Just make sure we are running
        '''
        servicemanager.LogInfoMsg("Service SvcDoRun Entered")
        self._logger.info("Starting the tasklist sensor . . . ")
        self.sensor.start()
        self._logger.info("Returned from Starting the tasklist sensor . . . ")


if __name__ == '__main__':
    if len(sys.argv) == 1:
        servicemanager.Initialize()
        servicemanager.PrepareToHostSingle(tasklist_service)        
        servicemanager.StartServiceCtrlDispatcher()
    else:
        sys.frozen = 'windows_exe'
        win32serviceutil.HandleCommandLine(tasklist_service)        
