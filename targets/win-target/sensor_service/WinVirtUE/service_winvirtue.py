import servicemanager
import socket
import sys
import win32event
import win32service
import win32serviceutil


class TestService1(win32serviceutil.ServiceFramework):
    _svc_name_ = "TestService1"
    _svc_display_name_ = "Test Service 1"
    _svc_description_ = "Test Service 1 Description"
    
    def __init__(self, args):
        win32serviceutil.ServiceFramework.__init__(self, args)
        self.hWaitStop = win32event.CreateEvent(None, 0, 0, None)
        socket.setdefaulttimeout(60)

    def SvcStop(self):
        self.ReportServiceStatus(win32service.SERVICE_STOP_PENDING)
        win32event.SetEvent(self.hWaitStop)

    def SvcDoRun(self):
        rc = None
        while rc != win32event.WAIT_OBJECT_0:
            with open('C:\\TestService1.log', 'a') as f:
                f.write('test TestService1 running...\n')
            rc = win32event.WaitForSingleObject(self.hWaitStop, 5000)

class TestService2(win32serviceutil.ServiceFramework):
    _svc_name_ = "TestService2"
    _svc_display_name_ = "Test Service 2"
    _svc_description_ = "Test Service 2 Description"

    def __init__(self, args):
        win32serviceutil.ServiceFramework.__init__(self, args)
        self.hWaitStop = win32event.CreateEvent(None, 0, 0, None)
        socket.setdefaulttimeout(60)

    def SvcStop(self):
        self.ReportServiceStatus(win32service.SERVICE_STOP_PENDING)
        win32event.SetEvent(self.hWaitStop)

    def SvcDoRun(self):
        rc = None
        while rc != win32event.WAIT_OBJECT_0:
            with open('C:\\TestService2.log', 'a') as f:
                f.write('test service 2 running...\n')
            rc = win32event.WaitForSingleObject(self.hWaitStop, 5000)
            
class TestService3(win32serviceutil.ServiceFramework):
    _svc_name_ = "TestService3"
    _svc_display_name_ = "Test Service 3"
    _svc_description_ = "Test Service 3 Description"

    def __init__(self, args):
        win32serviceutil.ServiceFramework.__init__(self, args)
        self.hWaitStop = win32event.CreateEvent(None, 0, 0, None)
        socket.setdefaulttimeout(60)

    def SvcStop(self):
        self.ReportServiceStatus(win32service.SERVICE_STOP_PENDING)
        win32event.SetEvent(self.hWaitStop)

    def SvcDoRun(self):
        rc = None
        while rc != win32event.WAIT_OBJECT_0:
            with open('C:\\TestService3.log', 'a') as f:
                f.write('test service 3 running...\n')
            rc = win32event.WaitForSingleObject(self.hWaitStop, 5000)            

if __name__ == '__main__':
    if len(sys.argv) == 1:
        servicemanager.Initialize()
        servicemanager.PrepareToHostMultiple("TestService1", TestService1)
        servicemanager.PrepareToHostMultiple("TestService2", TestService2)
        servicemanager.PrepareToHostMultiple("TestService3", TestService3)
        servicemanager.StartServiceCtrlDispatcher()
    else:
        win32serviceutil.HandleCommandLine(TestService1)
        win32serviceutil.HandleCommandLine(TestService2)
        win32serviceutil.HandleCommandLine(TestService3)