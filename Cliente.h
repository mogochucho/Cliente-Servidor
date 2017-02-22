/* 
 * File:   Cliente.h
 * Author: matias
 *
 * Created on 5 de octubre de 2015, 11:10 AM
 */

#ifndef CLIENTE_H
#define	CLIENTE_H

#include <netinet/in.h>
#include <unistd.h>

#include "SocketRDT.h"
#include "Sistema.h"

using namespace std;

class Mensaje {
public:
    string comando;
    string contenido;
};

class Cliente {
public:
    Cliente(struct sockaddr_in addr);
    Mensaje recibir();
    void enviar(const string msg);
    string getnick();
    virtual ~Cliente();
private:
    string nick;
    SocketRDT* skt;
};

#endif	/* CLIENTE_H */

