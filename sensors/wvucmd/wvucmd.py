'''
wvucmd.py
@brief command line interface used to test and interact with the Windows Virtue Device Driver
'''
import cmd
import logging


logger = logging.getLogger("ntfltmgr")
logger.addHandler(logging.NullHandler())
logger.setLevel(logging.ERROR)

class winVirtueCmd(cmd.Cmd):
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
    '''
    
    
    def connect(self):
        '''
        @brief Connect to the Windows Virtue Command Port
        '''
        pass
    
    def disconnect(self):
        '''
        @brief Disconnect from the Windows Virtue Driver Command Port
        '''
        
    
        
    
    FRIENDS = [ 'Alice', 'Adam', 'Barbara', 'Bob' ]
    
    def do_greet(self, person):
        "Greet the person"
        if person and person in self.FRIENDS:
            greeting = 'hi, %s!' % person
        elif person:
            greeting = "hello, " + person
        else:
            greeting = 'hello'
        print greeting
    
    def complete_greet(self, text, line, begidx, endidx):
        if not text:
            completions = self.FRIENDS[:]
        else:
            completions = [ f
                            for f in self.FRIENDS
                            if f.startswith(text)
                            ]
        return completions
    
    def do_EOF(self, line):
        return True

if __name__ == '__main__':
    HelloWorld().cmdloop()