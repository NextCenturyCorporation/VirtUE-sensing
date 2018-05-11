#!/usr/bin/python
import socket
import sys
import os
import subprocess

# Create a UDS socket
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

echo_response = subprocess.check_output('uname -r', shell=True)
print >> sys.stderr, "response expected: %s" % echo_response

# Connect the socket to the port where the server is listening
server_address = '/var/run/kernel_sensor'
print >>sys.stderr, 'connecting to %s' % server_address
try:
    sock.connect(server_address)
except socket.error, msg:
    print >>sys.stderr, msg
    sys.exit(1)

try:
    # Send data
    message = 'echo\0'
    print >>sys.stderr, 'sending "%s"' % message
    sock.sendall(message)

    amount_received = 0
    amount_expected = len(echo_response)

    while amount_received < amount_expected:
        data = sock.recv(amount_expected)
        amount_received += len(data)
        print >>sys.stderr, 'received "%s"' % data

finally:
    print >>sys.stderr, 'closing socket'
    sock.close()
