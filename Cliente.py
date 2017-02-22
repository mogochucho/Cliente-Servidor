
__author__ = 'matias'

from SocketRDT import SocketRDT as Socket
from ReciverMulticastRDT import ReciverMulticastRDT as Reciver
from threading import Thread

LOGIN = "LOGIN"
GOODBYE = "GOODBYE"


def wait_reciver(nick):
    reciver = Reciver(nick)
    while True:
        print reciver.recvRDT()


def wait_sender(skt, nick):
    skt.sendRDT(LOGIN + " " + nick)
    while True:
        skt.sendRDT(raw_input()+"\n")


if __name__ == '__main__':
    nick = " "
    while nick.find(" ") != -1:
        nick = raw_input("Ingrese un nick para el chat (el nick no puede contener espacios): ")

    skt = Socket()

    hiloReciver = Thread(target=wait_reciver, args=(nick,))
    hiloReciver.start()
    hiloEnviar = Thread(target=wait_sender, args=(skt,nick))
    hiloEnviar.start()

    salir = False

    while not salir:
        msg = skt.recvRDT()
        print msg
        if msg.find(GOODBYE) != -1:
            salir = True
    exit()
