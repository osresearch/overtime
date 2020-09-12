"""
Websockets client for micropython

Based very heavily off
https://github.com/aaugustin/websockets/blob/master/websockets/client.py
"""

import logging
import socket as socket
import binascii as binascii
import random as random
import ssl

from .protocol import Websocket, urlparse

LOGGER = logging.getLogger(__name__)


class WebsocketClient(Websocket):
    def __init__(self, uri):
        self.line = b''
        self.is_client = True

        uri = urlparse(uri)
        assert uri

        if __debug__: LOGGER.debug("open connection %s:%s",
        			    uri.hostname, uri.port)

        self.sock = socket.socket()
        addr = socket.getaddrinfo(uri.hostname, uri.port)
        self.sock.connect(addr[0][4])
        if uri.protocol == 'wss':
            self.sock = ssl.wrap_socket(self.sock)
            #self.sock.do_handshake()

        # Sec-WebSocket-Key is 16 bytes of random base64 encoded
        key = binascii.b2a_base64(bytes(random.getrandbits(8)
        				for _ in range(16)))[:-1]

        self.send_header('GET %s HTTP/1.1' % (uri.path or '/'))
        self.send_header('Upgrade: websocket')
        self.send_header('Host: %s:%s' % (uri.hostname, uri.port))
        self.send_header('Origin: http://%s:%s' % (uri.hostname, uri.port))
        self.send_header('Sec-WebSocket-Key: %s' % (str(key)[2:-1]))
        self.send_header('Sec-WebSocket-Version: 13')
        self.send_header('Connection: upgrade')
        self.send_header('')

        header = self.readline()
        assert header.startswith(b'HTTP/1.1 101 '), header

        # We don't (currently) need these headers
        # FIXME: should we check the return key?
        while header:
            if __debug__: LOGGER.debug(str(header))
            header = self.readline()

        #self.ws = Websocket(self.sock)
        super().__init__(self.sock)


    def send_header(self,line):
        if __debug__: LOGGER.debug("TX: '" + line + "'")
        self.sock.sendall(bytes(line + "\r\n", "UTF-8"))

#    def send(self,msg):
#        if __debug__: LOGGER.debug("TX: '" + msg + "'")
#        self.ws.send(msg)
#
#    def recv(self):
#        msg = self.ws.recv()
#        if __debug__ and msg is not None: LOGGER.debug("RX: '" + msg + "'")
#        return msg

    def readline(self):
        while True:
            offset = self.line.find(b'\r\n')
            if offset != -1:
                result = self.line[:offset] # do not include \r\n
                self.line = self.line[offset+2:] # skip the \r\n
                if __debug__: LOGGER.debug("RX: '" + str(result) + "'")
                return result

            msg = self.sock.recv(1)
            if msg is not None:
                self.line += msg

