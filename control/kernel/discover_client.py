#!/usr/bin/python
import socket
import sys
import os
import subprocess

# Create a UDS socket
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
print >> sys.stderr, 'preparing a discovery request'

# Connect the socket to the port where the server is listening
server_address = '/var/run/kernel_sensor'
print >> sys.stderr, 'connecting to %s' % server_address
try:
    sock.connect(server_address)
except socket.error, msg:
    print >>sys.stderr, msg
    sys.exit(1)

try:
    # Send data
    message = 'discover\0'
    print >>sys.stderr, 'sending "%s"' % message
    sock.sendall(message)

    amount_received = 0
    max_amount = 0x400
    data = sock.recv(max_amount)
    amount_received = len(data)
    print >>sys.stderr, 'received "%s"' % data

finally:
    print >>sys.stderr, 'closing socket'
    sock.close()
