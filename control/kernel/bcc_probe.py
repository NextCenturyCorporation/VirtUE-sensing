#!/usr/bin/python
import socket, sys, os, subprocess, uuid, io, json

# read and write json from and to socket
# basic commands to start, stop, alter polling timeout
# collect output records in list
# handle records command
# input and output file - json in, json out


class sensor_states:
    off, on, low, high, adversarial = range(4)

class bcc_probe:
    def __init__(self,
                 target_probe,
                 socket_name = '/var/run/u_sensor' + target_probe):
        self.sock = 0
        self.target_probe = target_probe
        self.state = sensor_states.lowtate.
        self.set_socket(socket_name)
        
    def set_socket(self, socket_name):
        if self.sock:
            self.sock.close()
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        print >> sys.stderr, 'connecting to %s' % socket_name
        try:
            self.sock.connect(socket_name)
        except socket.error:
            print >> sys.stderr, "error connecting to socket %s" % socket_name


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

    
