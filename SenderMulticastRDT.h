/* 
 * File:   SederMulticastRDT.h
 * Author: matias
 *
 * Created on 10 de octubre de 2015, 07:28 PM
 */

#ifndef SEDERMULTICASTRDT_H
#define	SEDERMULTICASTRDT_H
#define LEN_PKT_DEFOULT 1024

#include <set>
#include <string>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>

using namespace std;

class SenderMulticastRDT {
public:
    SenderMulticastRDT(int puerto_recivir, int puerto_multicast, const char* ip_multicast);
    void enviar(const string msg);
    void suscribir(const string key);
    void olvidar(const string key);
    virtual ~SenderMulticastRDT();
private:
    set<string> suscriptores;
    set<string> acks;
    list<string> mensajes;
    int desc_sender;
    int desc_reciver;
    sockaddr_in addr;
    sockaddr_in raddr;
    char rpkt[LEN_PKT_DEFOULT];
    char pkt[LEN_PKT_DEFOULT];
    int len_pkt;
    int len_rpkt;
    char seq_send;
    bool b_wait_acks;
    pthread_t id_hilo_reciver;
    pthread_t id_hilo_sender;
    pthread_mutex_t m_wait;
    pthread_cond_t cond_wait_acks;
    pthread_cond_t cond_acks;
    void udp_send(const char* pkt, int len);
    int udp_recv(char* pkt, int len, sockaddr_in& addr);
    static timespec gettimer();
    void terminar();
    void procesar_ack();
    void update_pkt();
    static void* reciver(void* mRDT);
    static void* sender(void* mRDT);
};

#endif	/* SEDERMULTICASTRDT_H */

