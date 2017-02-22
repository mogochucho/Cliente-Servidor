/* 
 * File:   SocketRDT.cpp
 * Author: matias
 * 
 * Created on 9 de octubre de 2015, 01:17 AM
 */

#include <stdlib.h>
#include <pthread.h>
#include <string>
#include <string.h>
#include <sys/time.h>
#include <iostream>

#include "SocketRDT.h"

using namespace std;

SocketRDT::SocketRDT(sockaddr_in addr) {
    // Creando socket
    this->descriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if(this->descriptor == -1){
        throw -1;
    }
    struct sockaddr_in local_addr;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = 0;
    if(bind(this->descriptor, (sockaddr*)&local_addr, sizeof(local_addr)) < 0){
        close(this->descriptor);
        throw -1;
    }
    
    // Inicializando vatiables de sincronizacion
    pthread_mutex_init(&(this->m_wait), NULL);
    pthread_cond_init(&(this->cond_recv), NULL);
    pthread_cond_init(&(this->cond_ack), NULL);
    pthread_cond_init(&(this->cond_wait_ack), NULL);
    pthread_cond_init(&(this->cond_wait), NULL);
    
    // inicializando variables de instancia
    this->b_wait_ack = false;
    this->b_wait_recv = false;
    this->seq_recv = (char) 1;
    this->seq_send = (char) 1;
    this->addr = addr;
    
    // Habilitando el receptor y sender
    if((pthread_create(&(this->id_hilo_wait), NULL, reciver, this) != 0) ||
            (pthread_create(&(this->id_hilo_wait_ack), NULL, sender, this) != 0)) {
        close(this->descriptor);
        throw -2;
    }
    
    // Conectando
    this->rdt_send("");
    this->rdt_recv(addr); // El primer mensaje se descarta
    pthread_mutex_lock(&(this->m_wait));
    this->addr = this->raddr; // Solo se usa para ser simetrico con el otro socket
    pthread_mutex_unlock(&(this->m_wait));
}

string SocketRDT::rdt_recv(sockaddr_in& addr) {
    pthread_mutex_lock(&(this->m_wait));
    if(this->b_wait_recv) {
        throw;
    }
    this->b_wait_recv = true;
    pthread_cond_wait(&(this->cond_recv), &(this->m_wait));
    addr = this->raddr;
    this->rpkt[this->len_rpkt] = '\0';
    string respuesta((this->rpkt)+3);
    this->seq_recv = this->rpkt[0];
    this->pkt[1] = this->seq_recv;
    this->b_wait_recv = false;
    pthread_cond_signal(&(this->cond_wait));
    pthread_mutex_unlock(&(this->m_wait));
    return respuesta;
}

void SocketRDT::rdt_send(const string& msg) {
    pthread_mutex_lock(&(this->m_wait));
    while(this->b_wait_ack) {
        pthread_cond_wait(&(this->cond_ack), &(this->m_wait));
    }
    this->b_wait_ack = true;
    strcpy((this->pkt)+3, msg.c_str());
    this->len_pkt = msg.length() + 3;
    this->seq_send = (this->seq_send == (char) 0? (char) 1 : (char) 0);
    this->pkt[0] = this->seq_send;
    this->pkt[1] = this->seq_recv;
    this->pkt[2] = (char) 1;
    pthread_cond_signal(&(this->cond_wait_ack));
    pthread_mutex_unlock(&(this->m_wait));
}

void* SocketRDT::reciver(void* socket) {
    SocketRDT* skt = (SocketRDT*) socket;
    while(true) {
        skt->len_rpkt = skt->udp_recv(skt->rpkt, LEN_PKT_DEFOULT-1, skt->raddr);
        pthread_mutex_lock(&(skt->m_wait));
        if(skt->len_rpkt >= 2) {
            if(skt->b_wait_ack && (skt->rpkt[1] == skt->seq_send)) {
                skt->procesar_ack();
            }
            if(skt->b_wait_recv && (skt->rpkt[0] != skt->seq_recv)) {
                pthread_cond_signal(&(skt->cond_recv));
                pthread_cond_wait(&(skt->cond_wait), &(skt->m_wait));
            }
            if(skt->b_wait_ack) {
                pthread_cond_signal(&(skt->cond_ack));
            } else if(skt->rpkt[2] == (char) 1) {
                skt->udp_send(skt->pkt, skt->len_pkt);
            }
        }
        pthread_mutex_unlock(&(skt->m_wait));
    }
}

void* SocketRDT::sender(void* socket) {
    SocketRDT* skt = (SocketRDT*) socket;
    timespec ts;
    while(true) {
        pthread_mutex_lock(&(skt->m_wait));
        if(!(skt->b_wait_ack)){
            pthread_cond_wait(&(skt->cond_wait_ack), &(skt->m_wait));
        }
        skt->udp_send(skt->pkt, skt->len_pkt);
        ts = skt->gettimer();
        pthread_cond_timedwait(&(skt->cond_ack), &(skt->m_wait), &ts);
        pthread_mutex_unlock(&(skt->m_wait));
    }
}

int SocketRDT::udp_recv(char* pkt, int len, sockaddr_in& addr) {
    socklen_t len_addr = sizeof(addr);
    return recvfrom(this->descriptor, (void*) pkt, len, 0, (sockaddr*) &addr, &len_addr);
}

void SocketRDT::udp_send(const char* pkt, int len) {
    sockaddr_in addr = this->addr;
    socklen_t len_addr = sizeof(addr);
    sendto(this->descriptor, pkt, len, 0, (sockaddr*) &addr, len_addr);
}

void SocketRDT::udp_send(const char* pkt, int len, sockaddr_in& addr) {
    socklen_t len_addr = sizeof(addr);
    sendto(this->descriptor, pkt, len, 0, (sockaddr*) &addr, len_addr);
}

void SocketRDT::procesar_ack() {
    this->b_wait_ack = false;
    this->pkt[2] = (char) 0;
    this->len_pkt = 3;
    pthread_cond_signal(&(this->cond_ack));
}

timespec SocketRDT::gettimer() {
    struct timespec   ts;
    struct timeval    tp;
    gettimeofday(&tp, NULL);
    ts.tv_sec  = tp.tv_sec + TIMEOUT_S;
    ts.tv_nsec = tp.tv_usec * 1000 + TIMEOUT_NS;
    return ts;
}

void SocketRDT::terminar() {
    pthread_cancel(this->id_hilo_wait);
    pthread_cancel(this->id_hilo_wait_ack);
    pthread_cond_destroy(&(this->cond_wait));
    pthread_cond_destroy(&(this->cond_wait_ack));
    pthread_cond_destroy(&(this->cond_ack));
    pthread_cond_destroy(&(this->cond_recv));
    pthread_mutex_destroy(&(this->m_wait));
    close(this->descriptor);
}

SocketRDT::~SocketRDT() {
    this->terminar();
}

