__author__ = 'matias'

from socket import socket as _socket, SOCK_DGRAM as _SOCK_DGRAM
from threading import Thread as _Thread, Lock as _Lock, Condition as _Condition

TIMEOUT = 30

class SocketRDT:
    def __init__(self, ip = 'localhost', puerto = 8080):
        """

        :param ip: IP del host con el que se desea conectar.
        :param puerto: Puerto del host con el que se desea conectar.
        """
        self._skt = _socket(type=_SOCK_DGRAM)
        self._skt.bind(('localhost', 0))
        self._addr = (ip, puerto)
        self._seq_recv = chr(1)
        self._seq_send = chr(1)
        self._b_wait_recv = False
        self._b_wait_send = False
        self._pkt = '' + self._seq_send + self._seq_recv + chr(0)
        self._rpkt = '' + self._seq_recv + self._seq_send + chr(0)
        self._m_wait = _Lock()
        self._cond_recv = _Condition()
        self._cond_sender = _Condition()
        self._cond_reciver = _Condition()
        self._cond_ack = _Condition()
        self._hilo_reciver = _Thread(target=self._reciver)
        self._hilo_sender = _Thread(target=self._sender)

        self._hilo_reciver.start()
        self._hilo_sender.start()

        self.sendRDT("")
        self.recvRDT()
        return

    def recvRDT(self):
        self._cond_recv.acquire()
        self._b_wait_recv = True
        self._cond_recv.wait()
        msg = self._rpkt[3:]
        self._seq_recv = self._rpkt[0]
        self._pkt = self._pkt[:1] + self._seq_recv + self._pkt[2:]
        self._b_wait_recv = False
        self._cond_reciver.acquire()
        self._cond_reciver.notify()
        self._cond_reciver.release()
        self._cond_recv.release()
        return msg

    def _reciver(self):
        while True:
            self._cond_reciver.acquire()
            self._rpkt, self._addr = self._skt.recvfrom(1023)
            if len(self._rpkt) >= 3:
                if self._b_wait_recv and (self._rpkt[0] != self._seq_recv):
                    self._cond_recv.acquire()
                    self._cond_recv.notify()
                    self._cond_recv.release()
                    self._cond_reciver.wait()
                if self._b_wait_send:
                    self._cond_ack.acquire()
                    if self._rpkt[1] == self._seq_send:
                        self._b_wait_send = False
                        self._pkt = self._pkt[:2] + chr(0) + self._pkt[3:]
                        self._cond_ack.notifyAll()
                    else:
                        self._cond_ack.notifyAll()
                    self._cond_ack.release()
                elif self._rpkt[2] == chr(1):
                    self._skt.sendto(self._pkt[:3], self._addr)
            self._cond_reciver.release()
        return

    def sendRDT(self, msg):
        """

        :type msg: str
        """
        self._cond_ack.acquire()
        while self._b_wait_send:
            self._cond_ack.wait()
        self._cond_sender.acquire()
        self._seq_send = chr(1) if self._seq_send == chr(0) else chr(0)
        self._pkt = self._seq_send + self._seq_recv + chr(1) + msg
        self._b_wait_send = True
        self._cond_sender.notify()
        self._cond_sender.release()
        self._cond_ack.release()
        return

    def _sender(self):
        while True:
            self._cond_sender.acquire()
            if not self._b_wait_send:
                self._cond_sender.wait()
            self._cond_ack.acquire()
            self._skt.sendto(self._pkt, self._addr)
            self._cond_ack.wait(TIMEOUT)
            self._cond_ack.release()
            self._cond_sender.release()
        return

