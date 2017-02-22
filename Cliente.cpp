/* 
 * File:   Cliente.cpp
 * Author: matias
 * 
 * Created on 5 de octubre de 2015, 11:10 AM
 */

#include "Cliente.h"
#include "Sistema.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#define LOGIN "LOGIN"

using namespace std;

Cliente::Cliente(sockaddr_in addr) {
    this->skt = new SocketRDT(addr);
    // Esperando Login
    Mensaje msg = this->recibir();
    if(msg.comando != LOGIN) {
        delete this->skt;
        throw -1;
    }
    this->nick = msg.contenido;
}

Mensaje Cliente::recibir() {
    sockaddr_in addr;
    string r(this->skt->rdt_recv(addr));
    cout << "IP: " << inet_ntoa(addr.sin_addr) << endl;
    cout << "Message: " << r << endl;
    Mensaje msg;
    int pos = r.find(" ");
    if(pos < 0) {
        msg.comando = r.substr(0, r.length()-1);
        msg.contenido = "";
    } else {
        msg.comando = r.substr(0, pos);
        msg.contenido = string(r.c_str()+pos+1);
    }
    return msg;
}

void Cliente::enviar(const string msg) {
    this->skt->rdt_send(msg);
}

string Cliente::getnick() {
    return this->nick;
}

Cliente::~Cliente() {
    delete this->skt;
}

