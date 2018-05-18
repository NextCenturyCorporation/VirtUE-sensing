#!/usr/bin/python

def connect_domain_sock(sockname): # Create a UDS socket
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

    # Connect the socket to the port where the server is listening

    print >> sys.stderr, 'connecting to %s' % sockname
    try:
        sock.connect(sockname)
        return sock

    except socket.error, msg:
        print >>sys.stderr, msg

def client_main(args):
    usage_string = """usage: %s [--connect] [--discover] [--echo]
                             [--socket <path>]""" % sys.argv[0]
    connect_string = "{Virtue-protocol-verion: 0.1}\n"
    socket_name = '/var/run/kernel_sensor'
    parser = argparse.ArgumentParser(description=usage_string)
    parser.add_argument("-s", "--socket",
                        nargs=1,
                        default=socket_name,
                        help="path to domain socket")
    parser.add_argument("-c", "--connect",
                        action="store_true",
                        help="issue a JSON connect message")
    parser.add_argument("-d", "--discover",
                        action='store_true',
                        help="retrieve a JSON array of loaded probes")
    parser.add_argument("-e", "--echo",
                        action='store_true',
                        help="test the controller's echo server")

    args = parser.parse_args()

    print >> sys.stderr args
# store the alternative socket name, if specified
    if args.socket != socket_name:
        socket_name = args.socket[0]

    s = False
    try:
        s = connect_domain_sock(socket_name)
    except:
        print >> sys.stderr, sys.exc_info()
    if s is False:
        print >> sys.stderr, "sock is false"


if __name__ == "__main__":
    import socket, sys, os, subprocess, argparse
    client_main(sys.argv)
    sys.exit(0)



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
