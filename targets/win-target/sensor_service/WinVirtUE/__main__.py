'''
@brief windows service top level __main__ file.  
'''
import os
import sys
import logging
import logging.handlers
from time import localtime, gmtime
from configparser import ConfigParser
import win32serviceutil
import servicemanager

from .service_winvirtue import WinVirtUE

__VERSION__ = "1.20180801"
__MODULE__  = "__main__.py"
logger = logging.getLogger(__name__)
cfgparser = ConfigParser()

def build_default_section_string(pkgbasedir):
    '''
    build a default section
    '''
    default_section = '''
[DEFAULT]
base_dir = {0}
config_dir = {1}
log_dir = {2}
cert_dir = {3}
    '''.format(pkgbasedir, os.path.join(pkgbasedir,"config"),
            os.path.join(pkgbasedir,"logs"), os.path.join(pkgbasedir,"certs"))
    return default_section

if __name__ == '__main__':
    basedir =  os.path.abspath(os.path.dirname(__file__))
    basedir += os.sep
    logfilename = os.path.join(basedir, "logs", __package__ + '.log')
    cfgfilename = os.path.join(basedir, "default.cfg")
    cfgparser = ConfigParser()
    cfgparser.read_string(build_default_section_string(basedir))
    cfgparser.read_file(open(cfgfilename, 'r'))
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
    
    if len(sys.argv) == 1:
        logger.info("Initializing ServiceManager . . .")
        servicemanager.Initialize()
        logger.info("Calling PrepareToHostSingle(WinVirtUE) . . .")
        servicemanager.PrepareToHostSingle(WinVirtUE)                
        logger.info("Calling StartServiceCtrlDispatcher() . . . ")
        servicemanager.StartServiceCtrlDispatcher()
    else:
        #sys.frozen = 'windows_exe'
        win32serviceutil.HandleCommandLine(WinVirtUE)     
