#!/usr/bin/python
import socket, sys, os, subprocess, uuid, io, json

# read and write json from and to socket
# basic commands to start, stop, alter polling timeout
# collect output records in list
# handle records command
# input and output file - json in, json out


# default is sensor_states.low
class sensor_states:
    low, high, adversarial = range(1,4)

class sensor_commands:
    connect, discovery, state, records, reset = range(1,6)

class bcc_probe:
    def __init__(self,
                 probe_name,
                 socket_name = '/var/run/bcc_sensor/' + probe_name,
                 out_file = '-'):

        self.target_probe = target_probe
        self.state = sensor_states.low
        self.sock = 0
        self.out_file = 0
        self.in_file = 0

        self.set_socket(socket_name)
        self.set_out_file(out_file)

    def set_socket(self, socket_name):
        if self.sock:
            self.sock.close()
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        print >> sys.stderr, 'connecting to %s' % socket_name
        try:
            self.sock.connect(socket_name)
        except socket.error:
            print >> sys.stderr, "error connecting to socket %s" % socket_name

    def set_out_file(self, out_file):
        print >> sys.stderr, "attempting to open %s as output file" % out
        if self.out_file and self.out_file != sys.stdout:
            self.out_file.close()
        if out and out != '-':
            self.out_file = open(out, "w+")
        else:
            self.out_file = sys.stdout

    def set_in_file(self, in_file):
        print >> sys.stderr, "attempting to open %s as input file" % in_file
        if self.in_file and self.in_file != sys.stdin:
            self.in_file.close()
        if in_file:
            self.in_file = open(in_file, "r")
        else:
            self.in_file = sys.stdin

"""
Commands are json objects like this:
{Virtue-protocol-verion: 0.1, request: [nonce, command, probe] }\n

they are loaded into a dictionary. the request element contains a list:
[nonce, command, probe_name]

request[command] is a dictionary:
{cmd: verb, parm: value}

Replies are json objects like this:

{Virtue-protocol-verion: 0.1, response: [nonce, result] }\n

the response element contains a list:

[nonce, result]

response[result] is a dictionary:

{cmd: verb, parm: value}

"""

# client_main is for testing, expect this class to be inherited by
# specific bcc_probes
def client_main(args):

    parser.add_argument("-f", "--file",
                        help = "output data to this file")
    parser.add_argument("-i", "--input",
                        help = "read input commands from this file")



if __name__ == "__main__":
    import argparse
    client_main(sys.argv)
    sys.exit(0)


