/* 
 * File:   Registro.cpp
 * Author: matias
 * 
 * Created on 5 de octubre de 2015, 11:23 AM
 */

#include <iostream>
#include <sys/time.h>
#include "Sistema.h"

using namespace std;

Sistema* Sistema::instance = NULL;

Sistema::Sistema() {
    sender = new SenderMulticastRDT(PUERTO_ACKS, PUERTO_MULTICAST, IP_MULTICAST);
    cantCliConectados = 0;
    cantMjsEnviados = 0;
    cantConexTotal = 0;
}

Sistema* Sistema::getSistema() {
    if(Sistema::instance == NULL) {
        Sistema::instance = new Sistema();
    }
    return Sistema::instance;
}

void Sistema::addCliente(const string nick, Cliente* cl) {
    pthread_mutex_lock(&(this->m_sistema));
    if(this->clientes.find(nick) != this->clientes.end()) {
        delete cl;
        throw -1;
    }
    this->clientes[nick].cl = cl;
    this->sender->suscribir(nick);
    if(pthread_create(&(this->clientes[nick].id_hilo), NULL, cliente_wait_timeout, (void*) &clientes[nick]) != 0) {
        throw -2;
    }
    if(pthread_create(&(this->clientes[nick].id_hilo), NULL, atender_cliente, (void*) &clientes[nick]) != 0) {
        throw -2;
    }
    (this->cantCliConectados)++;
    (this->cantConexTotal)++;
    pthread_mutex_unlock(&(this->m_sistema));
}

void Sistema::removeCliente(const string nick) {
    pthread_mutex_lock(&(this->m_sistema));
    if(this->clientes.find(nick) == this->clientes.end()){
        throw -1;
    }
    Cliente* cl = this->clientes[nick].cl;
    this->clientes.erase(nick);
    cl->enviar(string(GOODBYE) + "\n");
    delete cl;
    this->sender->olvidar(nick);
    (this->cantCliConectados)--;
    pthread_mutex_unlock(&(this->m_sistema));
}

void Sistema::enviar_a(const string origen, const string destino, const string mensaje) {
    string msg = string(PRIVATE_MESSAGE) + " " + origen + " " + mensaje;
    pthread_mutex_lock(&(this->m_sistema));
    if(this->clientes.find(destino) == this->clientes.end()){
        throw -1;
    }
    Cliente* cl = this->clientes[destino].cl;
    cl->enviar(msg);
    (this->cantMjsEnviados)++;
    pthread_mutex_unlock(&(this->m_sistema));
}

void Sistema::enviar_a_todos(const string origen, const string mensaje) {
    string msg = string(RELAYED_MESSAGE) + " " + origen + " " + mensaje;
    cout << "se intenta enviar: " << msg << endl;
    pthread_mutex_lock(&(this->m_sistema));
    this->sender->enviar(msg);
    pthread_mutex_unlock(&(this->m_sistema));
}

timespec Sistema::gettimer() {
    struct timespec   ts;
    struct timeval    tp;
    gettimeofday(&tp, NULL);
    ts.tv_sec  = tp.tv_sec + TIMEOUT_S;
    ts.tv_nsec = tp.tv_usec * 1000 + TIMEOUT_NS;
    return ts;
}

string Sistema::getStrConected() {
    string msg = CONNECTED;
    pthread_mutex_lock(&(this->m_sistema));
    for(map<string,DatosCliente>::iterator it = this->clientes.begin(); it != this->clientes.end(); ++it) {
        msg += " " +it->second.cl->getnick();
    }
    msg += "\n";
    pthread_mutex_unlock(&(this->m_sistema));
    return msg;
}

void* Sistema::atender_cliente(void* cliente) {
    DatosCliente cl = *(DatosCliente*) cliente;
    bool salir = false;
    Mensaje msg;
    while(!salir) {
        try {
            msg = cl.cl->recibir();
            pthread_cond_signal(&(cl.cond_notimeout));
            if(msg.comando == LOGOUT) {
                salir = true;
                Sistema::getSistema()->removeCliente(cl.cl->getnick());
            } else if(msg.comando == MESSAGE) {
                Sistema::getSistema()->enviar_a_todos(cl.cl->getnick(), msg.contenido);
                (Sistema::getSistema()->cantMjsEnviados)++;
            } else if(msg.comando == PRIVATE_MESSAGE) {
                int pos = msg.contenido.find(" ");
                string destino = msg.contenido.substr(0, pos);
                Sistema::getSistema()->enviar_a(cl.cl->getnick(),
                        destino,
                        string(msg.contenido.c_str()+pos+1));
                (Sistema::getSistema()->cantMjsEnviados)++;
            } else if(msg.comando == GET_CONNECTED) {
                cl.cl->enviar(Sistema::getSistema()->getStrConected());
            }
        } catch(int e) {
            if(e != -1) {
                throw -2;
            }
        }
    }
}

void* Sistema::cliente_wait_timeout(void* cliente) {
    DatosCliente cl = *((DatosCliente*) cliente);
    timespec ts = Sistema::gettimer();
    pthread_mutex_lock(&(cl.m_timeout));
    while(pthread_cond_timedwait(&(cl.cond_notimeout), &(cl.m_timeout), &ts) == 0) {
        ts = Sistema::gettimer();
    }
    pthread_mutex_unlock(&(cl.m_timeout));
    pthread_cancel(cl.id_hilo);
    pthread_cond_destroy(&(cl.cond_notimeout));
    pthread_mutex_destroy(&(cl.m_timeout));
    Sistema::getSistema()->removeCliente(cl.cl->getnick());
}

int Sistema::cantClientesConectados(){
    return this->cantCliConectados;
}

int Sistema::cantMensajesEnviados(){
    return this->cantMjsEnviados;
}

int Sistema::cantConexionesTotal(){
    return this->cantConexTotal;
}

Sistema::~Sistema() {
}

