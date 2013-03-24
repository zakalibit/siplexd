#!/usr/bin/env python
"USAGE: %s <port>"
from socket import *
from sys import argv

if len(argv) != 2:
    print __doc__ % argv[0]
else:
    sock = socket(AF_INET, SOCK_DGRAM)
    sock.bind(('',int(argv[1])))
    while 1:    # Run until cancelled
        message, client = sock.recvfrom(256) # <=256 byte datagram
        print message