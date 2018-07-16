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

        if self.pid:
            self.kill_proc_pid()
        else:
            self.kill_proc_name()

    def kill_proc_name(self):
        proc = subprocess.Popen(['pgrep', self.name],
                                stdout=subprocess.PIPE)
        for pid in proc.stdout:
            try:
                os.kill(int(pid), int(self.sig))
            except:
                continue

    def kill_proc_pid(self):
        try:
            os.kill(int(self.pid), int(self.sig))
        except:
            continue

def client_main(args):
    examples = """examples:
        ./KillProc.py -n emacs             # kill all processes named emacs with SIGTERM
        ./KillProc.py -p 1094              # kill pid 1094
        ./KillProc.py -p 1094 -s 4         # kill pid 1094 with SIGKILL
    """

    parser = argparse.ArgumentParser(
        description="Kill one or more Linux processes",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=examples)
    parser.add_argument("-n", "--name", help = "process name")
    parser.add_argument("-p", "--pid", help = "process id")
    parser.add_argument("-s", "--sig", nargs = '?', default = signal.SIGTERM,
                        help = "signal to send")
    args = parser.parse_args()
    response = KillProc(args)

if __name__ == "__main__":
    import argparse, sys
    client_main(sys.argv)
    sys.exit(0)
