# -*- coding: utf-8 -*-

import socket

UDP_PORT = 1555

sock = socket.socket()
sock.bind(('', UDP_PORT))
sock.listen(1)
conn, addr = sock.accept()
print 'connected:', addr

while True:
    data = conn.recv(1024)
    if not data:
        break
    conn.send(data.upper())

conn.close()    