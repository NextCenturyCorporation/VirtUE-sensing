import servicemanager
import socket
import sys
import win32event
import win32service
import win32serviceutil
from time import gmtime, strftime

class TestService1(win32serviceutil.ServiceFramework):
    _svc_name_ = "TestService1"
    _svc_display_name_ = "Test Service 1"
    _svc_description_ = "Test Service 1 Description"
    
    def __init__(self, args):
        win32serviceutil.ServiceFramework.__init__(self, args)
        self.hWaitStop = win32event.CreateEvent(None, 0, 0, None)
        socket.setdefaulttimeout(60)
        
    def SvcShutdown(self):
        servicemanager.LogInfoMsg("Doing something before shutdown")
    
    def SvcPause(self):
        self.ReportServiceStatus(win32service.SERVICE_PAUSE_PENDING)
        servicemanager.LogInfoMsg("Service State PAUSE PENDING")
        
        self.ReportServiceStatus(win32service.SERVICE_PAUSED)
        servicemanager.LogInfoMsg("Service State PAUSE")        
        
    def SvcContinue(self):
        self.ReportServiceStatus(win32service.SERVICE_CONTINUE_PENDING)
        servicemanager.LogInfoMsg("Service State CONTINUE PENDING")
        self.ReportServiceStatus(win32service.SERVICE_CONTROL_CONTINUE)
        servicemanager.LogInfoMsg("Service State CONTROL CONTINUE")
        self.ReportServiceStatus(win32service.SERVICE_RUNNING)
        servicemanager.LogInfoMsg("Service State RUNNING")

    def SvcStart(self):        
        servicemanager.LogInfoMsg("Service State START PENDING")
        self.ReportServiceStatus(win32service.SERVICE_START_PENDING)
        
    def SvcStop(self):
        win32event.SetEvent(self.hWaitStop)
        servicemanager.LogInfoMsg("Service State STOP PENDING")

    def SvcDoRun(self):
        rc = None
        while rc != win32event.WAIT_OBJECT_0:
            with open('C:\\TestService1.log', 'a') as f:
                f.write('{0}: test TestService1 running...\n'.format(strftime("%Y-%m-%d %H:%M:%S", gmtime()),))
            rc = win32event.WaitForSingleObject(self.hWaitStop, 5000)          

if __name__ == '__main__':
    if len(sys.argv) == 1:
        servicemanager.Initialize()
        servicemanager.PrepareToHostSingle(TestService1)        
        servicemanager.StartServiceCtrlDispatcher()
    else:
        win32serviceutil.HandleCommandLine(TestService1)        