#!"c:\program files\python\python36\python.exe"
"""
The Windows VirtUE Sensor Services
"""
import sys
import os
import socket

import logging
from uuid import uuid4
import logging
import logging.handlers
from time import localtime, gmtime
from configparser import ConfigParser

from threading import Thread

import win32serviceutil
import win32event
import servicemanager
import win32service

from sensor_winvirtue import sensor_winvirtue

__VERSION__ = "1.20180801"
__MODULE__ = "service_winvirtue.py"

logger = logging.getLogger(__name__)
logger.addHandler(logging.NullHandler())

class WinVirtUE_service(win32serviceutil.ServiceFramework):
    '''
    Python service that retrieves message from the kernel driver and
    then converts it to json and sends it on its way to the api
    '''

    _svc_name_ = "WinVirtUE Service"
    _svc_display_name_ = "Windows Virtue Service"
    _svc_description_ = "Windows Virtue Management Service"
    # This depends on the WinVirtUE driver service. Unfortunately a
    # bug (in win32serviceutil?) causes this string to be put in the
    # registry incorrectly, so we don't use it.
    #
    # Use instead: sc config "WinVirtUE Service" depend=WinVirtUE
    #_svc_deps_ = "WinVirtUE"
    
    def __init__(self, args):
        '''
        construct an instance of the WinVirtUE service
        '''
        win32serviceutil.ServiceFramework.__init__(self, args)        
        global cfgparser
        socket.setdefaulttimeout(60)
        self.hWaitStop = win32event.CreateEvent(None, 0, 0, None)
        servicemanager.LogInfoMsg("WinVirtUE Service Initializing")
        logger.info("WinVirtUE Service  . . . ")
        self._sensor = sensor_winvirtue(cfgparser)
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


def build_default_section_string(pkgbasedir):
    '''
    Build a default section. Exclude sensor_hostname, as we want
    sensor_wrapper's default behavior for that variable.
    '''
    virtue_id = str(uuid4())
    delay_start = 5
    if "USERNAME" in os.environ:
        username = os.environ["USERNAME"]
    api_retry_max=30.0
    api_retry_wait=0.5
    api_version='v1'
    default_section = '''
[DEFAULT]
base_dir = {0}
config_dir = {1}
log_dir = {2}
cert_dir = {3}
virtue_id = {4}
delay_start = {5}
username = {6}
api_retry_max = {7}
api_retry_wait = {8}
api_version = {9}
#sensor_hostname= {10} # excluded
api_https_port = 17504
api_http_port = 17141
sensor_advertised_hostname = None
sensor_advertised_port = None
check_for_long_blocking = False
backoff_delay = 30
    '''.format(pkgbasedir, os.path.join(pkgbasedir,"config"),
            os.path.join(pkgbasedir,"logs"), os.path.join(pkgbasedir,"certs"),
            virtue_id, delay_start, username, api_retry_max, api_retry_wait,
            api_version, socket.gethostname())
    return default_section


##
## This block gets executed whether we're invoking directly or via a service
##
if __name__ in ('__main__', 'service_winvirtue'):
    basedir =  os.path.abspath(os.path.dirname(__file__))
    basedir += os.sep
    logfilename = os.path.join(basedir, "logs", 'WinVirtUE.log')
    cfgfilename = os.path.join(basedir, "default.cfg")

    cfgparser = ConfigParser()
    cfgparser.read_file(open(cfgfilename, 'r'))

    if ("base_dir" not in cfgparser["DEFAULT"]
        or "config_dir" not in cfgparser["DEFAULT"]
        or "log_dir" not in cfgparser["DEFAULT"]
        or "cert_dir" not in cfgparser["DEFAULT"]
        or "virtue_id" not in cfgparser["DEFAULT"]
        or "delay_start" not in cfgparser["DEFAULT"]
        or "username" not in cfgparser["DEFAULT"]
        or "api_retry_max" not in cfgparser["DEFAULT"]
        or "api_retry_wait" not in cfgparser["DEFAULT"]
        or "api_version" not in cfgparser["DEFAULT"]):
        defsect = build_default_section_string(basedir)
        cfgparser.read_string(defsect)
    cfgparser.write(open(cfgfilename, 'w'))
    level = getattr(logging, cfgparser['LOGGING']['level'], logging.ERROR)
    utc = cfgparser.getboolean('LOGGING', 'utc')
    backupCount = cfgparser.getint('LOGGING', 'backupCount')
    # create our timed rotating file handler
    trfhandler = logging.handlers.TimedRotatingFileHandler(
        logfilename,
        when='midnight',
        interval=1,
        backupCount=backupCount,
        delay=False,
        utc=utc,
        atTime=None)
    # create the actual format
    fmt = logging.Formatter('%(asctime)s:%(name)s:%(levelname)s:%(message)s')
    # set the converter to either local or GMT (UTC, CUT, etc)
    setattr(fmt, 'converter', gmtime if utc is True else localtime)
    # set our handler with the updated formatter
    trfhandler.setFormatter(fmt)
    # All of the loggers will consult this config within this package
    logging.basicConfig(
        level       = level,               # configured level
        datefmt     = '%m-%d %H:%M',
        handlers    = [trfhandler])        # The timed rotating loggger
else:
    print("Script invoked in unexpected way")


##
## This block is executed only if we're calling directly (not via
## service)
##
if __name__ == '__main__':
    if len(sys.argv) == 1:
        logger.info("Initializing ServiceManager . . .")
        servicemanager.Initialize()
        logger.info("Calling PrepareToHostSingle(WinVirtUE) . . .")
        servicemanager.PrepareToHostSingle(WinVirtUE_service)
        logger.info("Calling StartServiceCtrlDispatcher() . . . ")
        servicemanager.StartServiceCtrlDispatcher()
    else:
        # The following line causes:
        #  - debugging to be easier (can run via
        #    python WinVirtUE\service_winvirtue.py debug)
        #  - the service to be installed so that Python.exe, rather than
        #    PythonServices.exe, is invoked upon service start (in other
        #    words this module won't work as a service)

        #sys.frozen = 'windows_exe'  # uncomment to allow debugging

        win32serviceutil.HandleCommandLine(WinVirtUE_service)
