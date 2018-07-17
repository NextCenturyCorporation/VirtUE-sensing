#!/usr/bin/python
#
# Copyright 2018 Michael D. Day II
# Copyright 2018 Two Six Labs
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.

###
### TODO: add a wildcard capability to the pgrep filter
###
import subprocess
import signal
import os

class KillProc:
    def __init__(self, args):
        self.name = args.name
        self.pid = args.pid
        self.sig = args.sig
        self.user = args.user
        self.parent = args.parent
        self.cmd = args.cmd

        if self.pid:
            self.kill_proc_pid()
        elif self.cmdline:
            self.kill_proc_cmd()
        elif self.user:
            self.kill_proc_user()
        elif self.parent:
            self.kill_proc_children()
        else:
            self.kill_proc_name()

    def kill_proc_name(self):
        try:
            proc = subprocess.Popen(['pkill', '--signal', self.sig, self.name])
        except OSError:
            pass

    def kill_proc_pid(self):
        try:
            os.kill(int(self.pid), int(self.sig))
        except OSError:
            pass

    def kill_proc_user(self):
        try:
            proc = subprocess.Popen(['pkill', '-u', self.user, '--signal', self.sig])
        except OSError:
            pass

    def kill_proc_children(self):
        try:
            proc = subprocess.Popen(['pkill', '-P', self.parent, '--signal', self.sig])
        except OSError:
            pass

    def kill_proc_cmd(self):
        try:
            proc = subprocess.Popen(['pkill', '-f', self.cmd, '--signal', self.sig])
        except OSError:
            pass

def client_main(args):
    examples = """examples:
        KillProc.py -n emacs             # kill all processes named emacs with SIGTERM
        KillProc.py -p 1094              # kill pid 1094
        KillProc.py -p 1094 -s 4         # kill pid 1094 with SIGKILL
        KillProc.py -u root              # kill processes owned by user root
        KillProc.py --parent 1           # kill children of init
        KillProc.py --cmd "/home/user/bin/" # kill matching command lines
    """

    parser = argparse.ArgumentParser(
        description="Kill one or more Linux processes",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=examples)
    parser.add_argument("-n", "--name", help = "process name")
    parser.add_argument("-u", "--user", help = "user name or id")
    parser.add_argument("-p", "--pid", help = "process id")
    parser.add_argument("--parent", help = "kill children of parent pid")
    parser.add_argument("--cmd", help = "kill command lines matching pattern")
    parser.add_argument("-s", "--sig", nargs = '?', default = "SIGTERM",
                        help = "signal to send")
    args = parser.parse_args()
    response = KillProc(args)

if __name__ == "__main__":
    import argparse, sys
    client_main(sys.argv)
    sys.exit(0)
