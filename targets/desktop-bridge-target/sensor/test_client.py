import socket
import sys
import time


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: {} <host> <port>".format(sys.argv[0]))
        print("Example: {} localhost 3434".format(sys.argv[0]))
        sys.exit(1)

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((sys.argv[1], int(sys.argv[2])))

    ct = 0
    while True:
        msg = "{\"test message\" : " + str(ct) + "}"
        print(msg)
        s.send(str.encode(msg + "\n"))
        ct += 1
        time.sleep(5.0)
        
    s.close()
