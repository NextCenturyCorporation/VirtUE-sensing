'''
wvucmd.py
@brief command line interface used to test and interact with the Windows Virtue Device Driver
'''
import cmd
import logging
import ntfltmgr
from ntfltmgr import FilterConnectCommunicationPort, EnumerateProbes, CloseHandle

logger = logging.getLogger("wvucmd")
logger.addHandler(logging.NullHandler())
logger.setLevel(logging.ERROR)

class WinVirtueCmd(cmd.Cmd):
    '''
    Windows Virtue Command Line Program
    @brief A command line python program control and manage the Windows Virtue Driver    
    Echo = 0x0
    EnableProtection  = 0x1
    DisableProtection = 0x2        
    EnableUnload = 0x3
    DisableUnload = 0x4
    EnumerateProbes = 0x5
    ConfigureProbe = 0x6
    OneShotKill = 0x7        
    ['SensorId', 'LastRunTime', 'RunInterval',  'OperationCount', 'Attributes', 'SensorName'])
    '''
    CommandPort = "\\WVUCommand"
    
    def do_kill(self, pid):
        '''
        @brief Not Implemented
        '''
        pass

    def do_configure(self, probe_id, cfgstr):
        '''
        @brief Not Implemented
        '''
        pass
    
    def do_disable_unload(self):
        '''
        @brief Not Implemented
        '''
        pass
    
    def do_enable_unload(self):
        '''
        
        @brief Not Implemented
        '''
        pass
    
    def do_disable_probe(self, sensor_name):
        '''
        @brief attempt to disable a registered probe
        @param sensor_name the sensors name that we will attempt to disable
        '''
        probe = None
        if sensor_name in self._probedict:
            probe = self._probedict[sensor_name]
            (res, _rspmsg,) = ntfltmgr.DisableProtection(probe.SensorId)
            logger.log(logging.info if res == 0 else logging.warning,
                       "Probe %s id %s has %sbeen Disabled",
                       probe.SensorName, probe.SensorId,
                       "" if res == 0 else "not")
        else:
            logger.warning("Attempting to disable a non-existant probe!")
            
    def complete_disable_probe(self, text, _line, _begidx, _endidx):
        '''
        completion logic for probe disable command
        '''
        if not text:
            completions = [ p.SensorName for p in self._probes]
        else:
            completions = [ p.SensorName
                            for p in self._probes
                            if p.startswith(text)
                            ]
        return completions    
    
    def do_enable_probe(self, sensor_name):
        '''
        @brief attempt to enable a registered probe
        @param sensor_name the sensors name that we will attempt to disable
        '''
        probe = None
        if sensor_name in self._probedict:
            probe = self._probedict[sensor_name]
            (res, _rspmsg,) = ntfltmgr.EnableProtection(probe.SensorId)                           
            logger.log(logging.info if res == 0 else logging.warning,
                       "Probe %s id %s has %sbeen Enabled",
                       probe.SensorName, probe.SensorId,
                       "" if res == 0 else "not")
        else:
            logger.warning("Attempting to enable a non-existant probe!")
                           
    def complete_enable_probe(self, text, _line, _begidx, _endidx):
        '''
        completion logic for probe enable command
        '''
        if not text:
            completions = [ p.SensorName for p in self._probes]
        else:
            completions = [ p.SensorName
                            for p in self._probes
                            if p.startswith(text)
                            ]
        return completions    
    
    def do_disconnect(self):
        '''
        @brief Disconnect from the Windows Virtue Driver Command Port
        '''
        if self._hFltComms:
            CloseHandle(self._hFltComms)
            self._hFltComms = None
    
    def do_list(self):
        '''
        Output a list of probes
        '''        
        for probe in self._probes:
            print("{0}".format(probe.SensorName,))
        
    def do_connect(self):
        '''
        @brief Connect to the Windows Virtue Command Port
        '''
        (_res, self._hFltComms,) = FilterConnectCommunicationPort(WinVirtueCmd.CommandPort)
        (_res, self._probes,) = EnumerateProbes(self._hFltComms)
        for probe in self._probes:
            self._probedict[probe.SensorName] = probe
            
    
    def __init__(self):
        '''
        construct an instance of this object
        '''
        super().__init__()
        self._hFltComms = None
        self._probes = None
        self._probedict = {}
        
    def cmdloop(self, intro=None):
        print( 'cmdloop(%s)' % intro)
        return super().cmdloop(intro)
    
    def preloop(self):
        print ('preloop()')
    
    def postloop(self):
        print ('postloop()')
        
    def parseline(self, line):
        print ('parseline(%s) =>' % line,)
        ret = super().parseline(line)
        print (ret)
        return ret
    
    def onecmd(self, line):
        print ('onecmd(%s)' % line)
        ret = super().onecmd(line)
        return ret

    def emptyline(self):
        print ('emptyline()')
        return super().emptyline()
    
    def default(self, line):
        print ('default(%s)' % line)
        return super().default(line)
    
    def precmd(self, line):
        print ('precmd(%s)' % line)
        return super().precmd(line)
    
    def postcmd(self, stop, line):
        print ('postcmd(%s, %s)' % (stop, line))
        return super().postcmd(stop, line)
    
    def do_EOF(self, _line):
        '''
        Exit from a CTRL-D
        '''
        return True
    

if __name__ == '__main__':
    WinVirtueCmd().cmdloop('Windows Virtue Driver Interface')
