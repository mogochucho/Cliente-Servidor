/* 
 * File:   SederMulticastRDT.cpp
 * Author: matias
 * 
 * Created on 10 de octubre de 2015, 07:28 PM
 */

#define TIMEOUT_S 60
#define TIMEOUT_NS 0

#include <arpa/inet.h>
#include <iostream>
#include <sys/time.h>
#include <string.h>
#include <list>
#include "SenderMulticastRDT.h"

using namespace std;

SenderMulticastRDT::SenderMulticastRDT(int puerto_recivir, int puerto_multicast, const char* ip_multicast) {
    // Creando socket para recibir acks
    this->desc_reciver = socket(AF_INET, SOCK_DGRAM, 0);
    if(this->desc_reciver == -1){
        throw -1;
    }
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(puerto_recivir);
    if(bind(this->desc_reciver, (sockaddr*)&addr, sizeof(addr)) < 0){
        close(this->desc_reciver);
        throw -1;
    }
    
    // Creando socket para enviar mensajes multicast
    this->desc_sender = socket(AF_INET, SOCK_DGRAM, 0);
    if(this->desc_sender == -1){
        throw -1;
    }
    this->addr.sin_addr.s_addr = inet_addr(ip_multicast);
    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(puerto_multicast);
    
    // Inicializando variables de instancia
    this->b_wait_acks = false;
    this->seq_send = (char) 1;
    
    // Inicializando vatiables de sincronizacion e hilo de escucha
    pthread_mutex_init(&(this->m_wait), NULL);
    if((pthread_create(&(this->id_hilo_reciver), NULL, reciver, this) != 0)
            || (pthread_create(&(this->id_hilo_sender), NULL, sender, this) != 0)) {
        pthread_mutex_destroy(&(this->m_wait));
        close(this->desc_reciver);
        throw -1;
    }
    pthread_cond_init(&(this->cond_wait_acks), NULL);
    pthread_cond_init(&(this->cond_acks), NULL);
}

void SenderMulticastRDT::suscribir(const string key) {
    pthread_mutex_lock(&(this->m_wait));
    this->suscriptores.insert(key);
    pthread_mutex_unlock(&(this->m_wait));
}

void SenderMulticastRDT::olvidar(const string key) {
    pthread_mutex_lock(&(this->m_wait));
    if(this->b_wait_acks) { // Si estaba esperando el ack con esa key la borra
        this->acks.erase(key);
        if(this->acks.empty()){
            this->update_pkt();
            pthread_cond_signal(&(this->cond_wait_acks));
        }
    }
    this->suscriptores.erase(key);
    pthread_mutex_unlock(&(this->m_wait));
}

void SenderMulticastRDT::enviar(const string msg) {
    pthread_mutex_lock(&(this->m_wait));
    this->mensajes.push_front(msg);
    if(!(this->b_wait_acks)){
        cout << "como todos recibieron el mensaje anterior entra aca" << endl;
        this->update_pkt();
        this->b_wait_acks = true;
        pthread_cond_signal(&(this->cond_wait_acks));
    }
    pthread_mutex_unlock(&(this->m_wait));
}

void* SenderMulticastRDT::reciver(void* mRDT) {
    SenderMulticastRDT* skt = (SenderMulticastRDT*) mRDT;
    while(true) {
        skt->len_rpkt = skt->udp_recv(skt->rpkt, LEN_PKT_DEFOULT-1, skt->raddr);
        pthread_mutex_lock(&(skt->m_wait));
        if(skt->len_rpkt >= 1) {
            if((skt->rpkt[0] == skt->seq_send) && skt->b_wait_acks) {
                skt->procesar_ack();
            }
        }
        pthread_mutex_unlock(&(skt->m_wait));
    }
    return NULL;
}

void* SenderMulticastRDT::sender(void* mRDT) {
    SenderMulticastRDT* skt = (SenderMulticastRDT*) mRDT;
    timespec ts;
    while(true) {
        pthread_mutex_lock(&(skt->m_wait));
        if(!(skt->b_wait_acks)) {
            pthread_cond_wait(&(skt->cond_wait_acks), &(skt->m_wait));
            cout << "se despierta el sender con la bandera en " << skt->b_wait_acks << endl;
        }
        skt->udp_send(skt->pkt, skt->len_pkt);
        ts = skt->gettimer();
        pthread_cond_timedwait(&(skt->cond_acks), &(skt->m_wait), &ts);
        pthread_mutex_unlock(&(skt->m_wait));
    }
    return NULL;
}

void SenderMulticastRDT::procesar_ack() {
    this->rpkt[this->len_rpkt] = '\0';
    this->acks.erase(string(this->rpkt + 1));
    if(this->acks.empty()) {
        this->update_pkt();
        pthread_cond_signal(&(this->cond_acks));
    }
}

void SenderMulticastRDT::update_pkt() {
    if(this->mensajes.empty()) {
        this->b_wait_acks = false;
    } else {
        strcpy((this->pkt)+1, this->mensajes.back().c_str());
        this->len_pkt = this->mensajes.back().length() + 1;
        this->seq_send = (this->seq_send == (char) 0? (char) 1 : (char) 0);
        this->pkt[0] = this->seq_send;
        this->mensajes.pop_back();
    }
}

int SenderMulticastRDT::udp_recv(char* pkt, int len, sockaddr_in& addr) {
    socklen_t len_addr = sizeof(addr);
    return recvfrom(this->desc_reciver, (void*) pkt, len, 0, (sockaddr*) &addr, &len_addr);
}

void SenderMulticastRDT::udp_send(const char* pkt, int len) {
    sendto(this->desc_sender, pkt, len, 0, (sockaddr*) &(this->addr), sizeof(this->addr));
}

timespec SenderMulticastRDT::gettimer() {
    struct timespec   ts;
    struct timeval    tp;
    gettimeofday(&tp, NULL);
    ts.tv_sec  = tp.tv_sec + TIMEOUT_S;
    ts.tv_nsec = tp.tv_usec * 1000 + TIMEOUT_NS;
    return ts;
}

void SenderMulticastRDT::terminar() {
    pthread_cancel(this->id_hilo_sender);
    pthread_cancel(this->id_hilo_reciver);
    pthread_cond_destroy(&(this->cond_acks));
    pthread_cond_destroy(&(this->cond_wait_acks));
    pthread_mutex_destroy(&(this->m_wait));
    close(this->desc_reciver);
}

SenderMulticastRDT::~SenderMulticastRDT() {
    this->terminar();
}

