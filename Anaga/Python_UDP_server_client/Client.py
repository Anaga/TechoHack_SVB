# -*- coding: utf-8 -*-

import socket
UDP_PORT = 1555
MASTER_IP = '192.168.0.103'

sock = socket.socket()
sock.connect((MASTER_IP, UDP_PORT))
sock.send('hello, world!')

data = sock.recv(1024)
sock.close()

print data

