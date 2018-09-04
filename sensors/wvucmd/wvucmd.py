'''
wvucmd.py
@brief command line interface used to test and interact with the Windows Virtue Device Driver
'''
import cmd
import sys
import logging
import colorama
import ntfltmgr

from ntfltmgr import FilterConnectCommunicationPort, EnumerateSensors
from ntfltmgr import CloseHandle, packet_decode, EventPort, CommandPort

colorama.init()

logger = logging.getLogger("wvucmd")
logger.addHandler(logging.NullHandler())
logger.setLevel(logging.ERROR)

class WinVirtueCmd(cmd.Cmd):
    '''
    Windows Virtue Command Line Program
    @brief A command line python program control and manage the Windows Virtue Driver    
    Echo = 0x0
    EnableProbe  = 0x1
    DisableProbe = 0x2        
    EnableUnload = 0x3
    DisableUnload = 0x4
    EnumerateSensors = 0x5
    ConfigureProbe = 0x6
    OneShotKill = 0x7        
    '''

    
    def do_kill(self, _pid):
        '''
        @brief Not Implemented
        '''
        if self._connected is False:
            logger.warning("Not Connected!")
            return

    def do_configure(self, _probe_id, _cfgstr):
        '''
        @brief Not Implemented
        '''
        if self._connected is False:
            logger.warning("Not Connected!")
            return
    
    def do_disable_unload(self, _line):
        '''
        @brief Not Implemented
        '''
        if self._connected is False:
            logger.warning("Not Connected!")
            return
    
    def do_enable_unload(self, _line):
        '''
        
        @brief Not Implemented
        '''
        if self._connected is False:
            logger.warning("Not Connected!")
            return
    
    def do_disable_probe(self, sensor_name):
        '''
        @brief attempt to disable a registered probe
        @param sensor_name the sensors name that we will attempt to disable
        '''
        if self._connected is False:
            logger.warning("Not Connected!")
            return

        probe = None
        if sensor_name == "All":
            (res, _rspmsg,) = ntfltmgr.DisableProbe(self._hFltComms, "All")
            logger.log(logging.INFO if res == 0 else logging.WARNING,
                               "All Probes have %sbeen Disabled",
                               "" if res == 0 else " not")
            return
            
        if sensor_name in self._probedict:
            probe = self._probedict[sensor_name]
            (res, _rspms,) = ntfltmgr.DisableProbe(self._hFltComms, 
                    probe.SensorId)
            logger.log(logging.INFO if res == 0 else logging.WARNING,
                       "Probe %s id %s has %sbeen Disabled",
                       probe.SensorName, probe.SensorId,
                       "" if res == 0 else "not ")
        else:
            logger.warning("Attempting to disable a non-existant probe!")
            
    def complete_disable_probe(self, text, _line, _begidx, _endidx):
        '''
        completion logic for probe disable command
        '''
        if not text:
            completions = [ p.SensorName for p in self._probes]
            completions.insert(0, "All")
        else:
            completions = [ p.SensorName
                            for p in self._probes
                            if p.SensorName.startswith(text)
                            ]
        return completions    
    
    def do_enable_probe(self, sensor_name):
        '''
        @brief attempt to enable a registered probe
        @param sensor_name the sensors name that we will attempt to disable
        '''
        if self._connected is False:
            logger.warning("Not Connected!")
            return

        probe = None
        if sensor_name == "All":
            (res, _rspmsg,) = ntfltmgr.EnableProbe(self._hFltComms, "All")
            logger.log(logging.INFO if res == 0 else logging.WARNING,
                               "All Probes have %sbeen Enabled",
                                       "" if res == 0 else " not")
            return
        
        if sensor_name in self._probedict:
            probe = self._probedict[sensor_name]
            (res, _rspmsg,) = ntfltmgr.EnableProbe(self._hFltComms, 
                    probe.SensorId)
            logger.log(logging.INFO if res == 0 else logging.WARNING,
                       "Probe %s id %s has %sbeen Enabled",
                       probe.SensorName, probe.SensorId,
                       "" if res == 0 else " not")
        else:
            logger.warning("Attempting to enable a non-existant probe!")
                           
    def complete_enable_probe(self, text, _line, _begidx, _endidx):
        '''
        completion logic for probe enable command
        '''
        if not text:
            completions = [ p.SensorName for p in self._probes]
            completions.insert(0, "All")
        else:
            completions = [ p.SensorName
                            for p in self._probes
                            if p.SensorName.startswith(text)
                            ]
        return completions    
    
    def do_disconnect(self, _line):
        '''
        @brief Disconnect from the Windows Virtue Driver Command Port
        '''
        self._connected = False
        self.prompt = "wvucmd [disconnected]: "
        if self._hFltComms:
            CloseHandle(self._hFltComms)
            self._hFltComms = None
    
    def do_list(self, _line):
        '''
        Output a list of probes
        '''        
        if self._connected is False:
            logger.warning("Not Connected!")
            return
        (_res, self._probes,) = EnumerateSensors(self._hFltComms)
        for probe in self._probes:
            self._probedict[probe.SensorName] = probe
        field_list = ['SensorId', 'LastRunTime', 'RunInterval',  'OperationCount', 'Attributes', 'Enabled', 'SensorName']
        row_format ="{:<15}{:<26}{:<15}{:<13}{:<15}{:^32}{:<4}{:<15}"
        print(row_format.format("", *field_list))
        for row in self._probes:
            print(*row)
        
    def do_connect(self, _like):
        '''
        @brief Connect to the Windows Virtue Command Port
        '''
        (_res, self._hFltComms,) = FilterConnectCommunicationPort(CommandPort)
        (_res, self._probes,) = EnumerateSensors(self._hFltComms)
        for probe in self._probes:
            self._probedict[probe.SensorName] = probe
        self._connected = True
        self.prompt = "wvucmd [connected]: "
            
    def do_dump(self, outfile):
        '''
        @brief dump the event queue from the driver to a file
        @param outfile name of output file default is 'outfile.txt', will overwrite
        file of same name.
        '''
        if not outfile:
            outfile='outfile.txt'

        (_res, hFltComms,) = FilterConnectCommunicationPort(EventPort)
        try:
            with open(outfile,"w") as of:
                for pkt in packet_decode():
                    of.write(pkt)
        except (KeyboardInterrupt, SystemExit) as _err:
            logger.exception("do_dump terminated by error {0}, _err")
        finally:
            CloseHandle(hFltComms) 

    def __init__(self):
        '''
        construct an instance of this object
        '''
        super().__init__()
        self._hFltComms = None
        self._probes = None
        self._probedict = {}
        self._connected = False
        self.prompt = "wvucmd [disconnected]: "

    def do_EOF(self, _line):
        '''
        Exit from a CTRL-D
        '''
        return True
    
    def do_quit(self, _line):
        '''
        Exit 
        '''
        if (self._connected is True and self._hFltComms):
            CloseHandle(self._hFltComms)
        sys.exit(0)
    do_exit = do_quit
    