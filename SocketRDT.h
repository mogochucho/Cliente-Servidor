/* 
 * File:   SocketRDT.h
 * Author: matias
 *
 * Created on 9 de octubre de 2015, 01:17 AM
 */

#ifndef SOCKETRDT_H
#define	SOCKETRDT_H

#define TIMEOUT_S 60
#define TIMEOUT_NS 0
#define LEN_PKT_DEFOULT 1024

#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <list>
#include <sys/types.h>

using namespace std;

class SocketRDT {
public:
    SocketRDT(sockaddr_in addr);
    string rdt_recv(sockaddr_in& addr);
    void rdt_send(const string& msg);
    virtual ~SocketRDT();
private:
    int descriptor;
    sockaddr_in addr;
    sockaddr_in raddr;
    char seq_recv;
    char seq_send;
    char pkt[LEN_PKT_DEFOULT];
    char rpkt[LEN_PKT_DEFOULT];
    int len_pkt;
    int len_rpkt;
    pthread_mutex_t m_wait;
    pthread_cond_t cond_ack;
    pthread_cond_t cond_wait_ack;
    pthread_cond_t cond_recv;
    pthread_cond_t cond_wait;
    bool b_wait_ack;
    bool b_wait_recv;
    pthread_t id_hilo_wait;
    pthread_t id_hilo_wait_ack;
    void udp_send(const char* pkt, int len);
    void udp_send(const char* pkt, int len, sockaddr_in& addr);
    int udp_recv(char* pkt, int len, sockaddr_in& addr);
    static void* reciver(void* socket);
    static void* sender(void* socket);
    void procesar_ack();
    static timespec gettimer();
    void terminar();
};

#endif	/* SOCKETRDT_H */

