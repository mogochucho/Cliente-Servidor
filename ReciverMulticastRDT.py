__author__ = 'matias'

from socket import _socket
import struct as _struct
from threading import Thread as _Thread, Lock as _Lock, Condition as _Condition

IP_MULTICAST = '225.5.4.85'
PUERTO_MULTICAST = 1234
IP_ACKS = 'localhost'
PUERTO_ACKS = 9090

class ReciverMulticastRDT:
    def __init__(self, key, ipMulticast = IP_MULTICAST, puertoMulticast = PUERTO_MULTICAST,
                 ipAcks = IP_ACKS, puertoAcks = PUERTO_ACKS):
        """

        :param key: Llave que identifica al suscriptor
        :type key: str
        :param ipMulticast: IP a la cual suscribirse para escuchar los mensajes multicast
        :type ipMulticast: str
        :param puertoMulticast: puerto en el cual se esperan los mensajes multicast
        :type puertoMulticast: int
        :param ipAcks: IP a la cual se envian los acks
        :type ipAcks: str
        :param puertoAcks: puerto al cual se envian los acks
        :type puertoAcks: int
        """
        self._skt = _socket.socket(_socket.AF_INET, _socket.SOCK_DGRAM, _socket.IPPROTO_UDP)
        self._skt.setsockopt(_socket.SOL_SOCKET, _socket.SO_REUSEADDR, 1)
        self._skt.bind(('', puertoMulticast))
        mreq = _struct.pack("=4sl", _socket.inet_aton(ipMulticast), _socket.INADDR_ANY)
        self._skt.setsockopt(_socket.IPPROTO_IP, _socket.IP_ADD_MEMBERSHIP, mreq)

        self._key = key
        self._b_wait_recv = False
        self._seq_recv = chr(2)
        self._cond_recv = _Condition()
        self._cond_reciver = _Condition()
        self._addr = (ipAcks, puertoAcks)
        self._hilo_reciver = _Thread(target=self._reciver)

        self._hilo_reciver.start()
        return

    def recvRDT(self):
        self._cond_recv.acquire()
        self._b_wait_recv = True
        self._cond_recv.wait()
        msg = self._rpkt[1:]
        self._seq_recv = self._rpkt[0]
        self._b_wait_recv = False
        self._cond_reciver.acquire()
        self._cond_reciver.notify()
        self._cond_reciver.release()
        self._cond_recv.release()
        return msg

    def _reciver(self):
        while True:
            self._cond_reciver.acquire()
            self._rpkt = self._skt.recvfrom(1023)[0]
            if len(self._rpkt) >= 1 and (self._rpkt[0] != self._seq_recv):
                if self._b_wait_recv:
                    self._cond_recv.acquire()
                    self._cond_recv.notify()
                    self._cond_recv.release()
                    self._cond_reciver.wait()
            self._skt.sendto(self._seq_recv + self._key, self._addr)
            self._cond_reciver.release()
        return

if __name__ == '__main__':
    sock = _socket.socket(_socket.AF_INET, _socket.SOCK_DGRAM, _socket.IPPROTO_UDP)
    sock.setsockopt(_socket.SOL_SOCKET, _socket.SO_REUSEADDR, 1)
    sock.bind(('', PUERTO_MULTICAST))
    # wrong: mreq = struct.pack("sl", _socket.inet_aton("224.51.105.104"), _socket.INADDR_ANY)
    mreq = _struct.pack("=4sl", _socket.inet_aton(IP_MULTICAST), _socket.INADDR_ANY)
    sock.setsockopt(_socket.IPPROTO_IP, _socket.IP_ADD_MEMBERSHIP, mreq)
    input()
    rm = ReciverMulticastRDT("koko")
    input()
