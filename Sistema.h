/* 
 * File:   Sistema.h
 * Author: matias
 *
 * Created on 5 de octubre de 2015, 11:50 AM
 */

#ifndef SISTEMA_H
#define	SISTEMA_H

#include <map>
#include <list>
#include "Cliente.h"
#include "SenderMulticastRDT.h"
#define IP_MULTICAST "225.5.4.85"
#define PUERTO_MULTICAST 1234
#define PUERTO_ACKS 9090
#define PUERTO_WS 8080
#define LOGOUT "LOGOUT"
#define GET_CONNECTED "GET_CONNECTED"
#define MESSAGE "MESSAGE"
#define PRIVATE_MESSAGE "PRIVATE_MESSAGE"
#define RELAYED_MESSAGE "RELAYED_MESSAGE"
#define CONNECTED "CONNECTED"
#define GOODBYE "GOODBYE"
#define TIMEOUT_CLIENTE_S 900
#define TIMEOUT_CLIENTE_NS 0

using namespace std;

class Cliente;

struct DatosCliente {
    Cliente* cl;
    pthread_t id_hilo;
    pthread_t id_hilo_timeout;
    pthread_cond_t cond_notimeout;
    pthread_mutex_t m_timeout;
};

class Sistema {
public:
    static Sistema* getSistema();
    void enviar_a(const string origen, const string destino, const string msg);
    void enviar_a_todos(const string origen, const string msg);
    string getStrConected();
    void addCliente(const string nick, Cliente* cl);
    void removeCliente(const string nick);
    int cantClientesConectados();
    int cantMensajesEnviados();
    int cantConexionesTotal();
private:
    Sistema();
    static Sistema* instance;
    SenderMulticastRDT* sender;
    map<string, DatosCliente> clientes;
    list<string> mensajes;
    pthread_mutex_t m_sistema;
    static void* atender_cliente(void* cliente);
    static void* cliente_wait_timeout(void* cliente);
    static timespec gettimer();
    virtual ~Sistema();
    // Contadores
    int cantCliConectados;
    int cantMjsEnviados;
    int cantConexTotal;
};

#endif	/* SISTEMA_H */

