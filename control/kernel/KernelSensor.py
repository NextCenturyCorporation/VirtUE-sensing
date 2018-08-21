#!/usr/bin/python
import socket, sys, os, subprocess, uuid, io, json

class KernelSensor:
# TODO: self.connect_string is currently unused
    connect_string = "{Virtue-protocol-verion: 0.1}\n"
    def __init__(self,
                 args,
                 socket_name = '/var/run/kernel_sensor',
                 target_sensor = '"Kernel PS Sensor"',
                 out_file = '-'):
        self.args = args
        self.sock = 0
        self.out_file = 0
        self.in_file = 0
        self.target_sensor = target_sensor
        self.set_socket(socket_name)
        self.set_out_file(out_file)
        self.protocol_version = 0.1

    def set_socket(self, socket_name):
        if self.sock:
            self.sock.close()
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        print >> sys.stderr, 'connecting to %s' % socket_name
        try:
            self.sock.connect(socket_name)
        except socket.error:
            print >> sys.stderr, "error connecting to socket %s" % socket_name

    def set_out_file(self, out):
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

    def set_target_sensor(self, sensor):
        self.target_sensor = sensor

    def run_input_file(self):
        # each line in the file is expected to be a json object that contains
        # a request message
        for line in self.in_file:
            try:
                request = json.loads(line)
                try:
                    self.sock.sendall(json.dumps(request))
                    self.sock.settimeout(0.5)
                    amount_received = 0
                    max_amount = 0x400
                    data = self.sock.recv(max_amount)
                    while len(data):
                        self.out_file.write("%s" %(data))
                        data = self.sock.recv(max_amount)
                        self.out_file.write("%s" %(data))

                except:
                    print >> sys.stderr, "run_input_file: closing socket"
                    self.sock.close()
            except:
                print >> sys.stderr, "Not a valid JSON object %s" %(line)

    def json_connect(self):
        try:
            message = "{Virtue-protocol-version: 0.1}\n\0"
            print >>sys.stderr, 'sending "%s"' % message
            self.sock.sendall(message)

            amount_received = 0
            max_amount = 0x400

            data = self.sock.recv(max_amount)
            amount_received = len(data)
            print >>sys.stderr, 'received "%s"' % data
        except:
            print >>sys.stderr, 'json_connect: closing socket'
            self.sock.close()

    def send_echo_test(self):
        echo_response = subprocess.check_output('uname -r', shell=True)
        print >> sys.stderr, "response expected: %s" % echo_response

        try:
            message = 'echo\0'
            print >>sys.stderr, 'sending "%s"' % message
            self.sock.sendall(message)

            amount_received = 0
            max_amount = 0x400

            data = self.sock.recv(max_amount)
            amount_received = len(data)
            print >>sys.stderr, 'received "%s"' % data
        except:
            print >>sys.stderr, 'send_discover_test: closing socket'
            self.sock.close()


# send a discovery test message
# not a proper json discover message, but tests building
# a list of all running probes and returning that list as a json array
    def send_discovery_test(self):
        try:
            message = 'discover\0'
            print >>sys.stderr, 'sending "%s"' % message
            self.sock.sendall(message)

            amount_received = 0
            max_amount = 0x400
            data = self.sock.recv(max_amount)
            amount_received = len(data)
            print >>sys.stderr, 'discovery test received "%s"' % data

        except:
            print >>sys.stderr, 'send_discover_test: closing socket'
            self.sock.close()

# send a proper discovery protocol message, with the
# header, messsage type, nonce, and command
# '{Virtue-protocol-verion: 0.1, request: [nonce, discovery, *]}'
    def send_discovery_message(self):
       try:
           json_message = {'Virtue-protocol-version': self.protocol_version,
                           'request': [ uuid.uuid4().hex, 'discovery', '*' ]}

           print >> sys.stderr, json.dumps(json_message, sort_keys = True)
           self.sock.sendall(json.dumps(json_message, sort_keys = True))

           amount_received = 0
           max_amount = 0x400
           data = self.sock.recv(max_amount)
           amount_received = len(data)
           self.out_file.write("%s" %(data))

       except:
           print >>sys.stderr, 'send_discovery_message: closing socket'
           self.sock.close()

    def send_state_message(self):
        try:
            message_header = "{Virtue-protocol-version: 0.1, request: ["
            message_nonce = uuid.uuid4().hex
            message_command = self.args.state
            message_footer = "]}"
            print >> sys.stderr, 'sending "%s%s, %s, %s%s"' \
                %(message_header, message_nonce, message_command, \
                  self.target_sensor, message_footer)
            self.sock.sendall("%s%s, %s, %s%s" \
                              %(message_header, message_nonce, message_command, \
                                self.target_sensor, message_footer))

            self.sock.settimeout(0.5)
            amount_received = 0
            max_amount = 0x400
            data = self.sock.recv(max_amount)
            while len(data):
                self.out_file.write("%s" %(data))
                data = self.sock.recv(max_amount)

        except:
            print >>sys.stderr, 'send_records_message: closing socket'
            self.sock.close()
    def send_records_message(self):
        try:
            message_header = "{Virtue-protocol-version: 0.1, request: ["
            message_nonce = uuid.uuid4().hex
            message_command = ", records, "
            message_footer = "]}"
            print >> sys.stderr, 'sending "%s%s%s%s%s"' \
                %(message_header, message_nonce, message_command, \
                  self.target_sensor, message_footer)
            self.sock.sendall("%s%s%s%s%s" \
                              %(message_header, message_nonce, message_command, \
                                self.target_sensor, message_footer))

            self.sock.settimeout(0.5)
            amount_received = 0
            max_amount = 0x400
            data = self.sock.recv(max_amount)
            while len(data):
                self.out_file.write("%s" %(data))
                data = self.sock.recv(max_amount)

        except:
            print >>sys.stderr, 'send_records_message: closing socket'
            self.sock.close()


def client_main(args):
    usage_string = """usage: %s [--connect] [--discover] [--echo]
                             [--socket <path>] [...]""" % sys.argv[0]
    connect_string = "{Virtue-protocol-verion: 0.1}\n"
    parser = argparse.ArgumentParser(description=usage_string)
    parser.add_argument("-s", "--socket",
                        help = "path to domain socket")
    parser.add_argument("-c", "--connect",
                        action = "store_true",
                        help = "issue a JSON connect message")
    parser.add_argument("-d", "--discover",
                        action = 'store_true',
                        help = "retrieve a JSON array of loaded probes (not a full json exchange)")
    parser.add_argument("-e", "--echo",
                        action = 'store_true',
                        help = "test the controller's echo server")
    parser.add_argument("-r", "--records",
                        action = 'store_true',
                        help = "test the controller's echo server")
    parser.add_argument("--state",
                        help = "send a state change message (off, on, low, high, etc.)")
    parser.add_argument("--sensor",
                        help = "target sensor for message")
    parser.add_argument("-o", "--outfile",
                        help = "output data to this file")
    parser.add_argument("-i", "--infile",
                        help = "read input commands from this file")

    args = parser.parse_args()
    sensor = KernelSensor(args)

    if args.socket:
        sensor.set_socket(args.socket)
    if args.sensor:
        sensor.set_target_sensor(args.sensor)
    if args.outfile:
        sensor.set_out_file(args.outfile)
    if args.infile:
        sensor.set_in_file(args.infile)
        sensor.run_input_file()
        sys.exit(0)

    if args.state:
        sensor.send_state_message()

    if args.records:
        sensor.send_records_message()

    if args.discover:
        sensor.send_discovery_message()

    if args.echo:
        sensor.send_echo_test()

    if args.connect:
        sensor.json_connect()

if __name__ == "__main__":
    import argparse
    client_main(sys.argv)
    sys.exit(0)


