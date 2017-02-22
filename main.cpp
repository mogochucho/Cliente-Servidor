/* 
 * File:   main.cpp
 * Author: matias
 *
 * Created on 5 de octubre de 2015, 11:03 AM
 */

#include <iostream>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "WellcomeSocket.h"
#include "Sistema.h"
#include "SenderMulticastRDT.h"

using namespace std;

void* addCliente(void* cl);
void* comandosConsola(void* timeIni);
int a;

/*
 * 
 */
int main(int argc, char** argv) {
    // Inicializando variables globales
    time_t timeIni = time(NULL);
    
    // Inicializando variables locales
    pthread_t idHilo;
    WellcomeSocket ws(PUERTO_WS);
    sockaddr_in* addr = NULL;
    
    pthread_t idHiloConsola;
    pthread_create(&idHiloConsola, NULL, comandosConsola, (void *) timeIni);    
    
    // Bucle principal
    while(true) {
        addr = ws.aceptar();
        if(pthread_create(&idHilo, NULL, addCliente, (void*) addr) != 0) {
            return -2;
        }
    }

    return 0;
}

void* addCliente(void* addr) {
    Cliente* cl;
    try {
        cl = new Cliente(*((sockaddr_in*) addr));
    } catch(int e) {
        if(e != -1)
            throw -2;
        free(addr);
        return NULL;
    }
    free(addr);
    try {
        Sistema::getSistema()->addCliente(cl->getnick(), cl);
        cl->enviar("se agrego el cliente al sistema");
    } catch(int e) {
        if(e != -1)
            throw -2;
    }
    return NULL;
}

void* comandosConsola(void* timeIni){
    char* comando = (char *) malloc(sizeof(char));
    
    while(true) {
        // Lectura entrada desde consola
        int a = scanf("%s",comando);
        if (strcmp(comando,"a") == 0) {
            cout << "Cantidad de clientes conectados: " << Sistema::getSistema()->cantClientesConectados() << endl;
        }else if (strcmp(comando,"s") == 0) {
            cout << "Cantidad de mensajes enviados: " << Sistema::getSistema()->cantMensajesEnviados() << endl;
        }else if (strcmp(comando,"d") == 0) {
            cout << "Cantidad de conexiones totales: " << Sistema::getSistema()->cantConexionesTotal() << endl;
        }else if (strcmp(comando,"f") == 0) {
                cout << "Tiempo de ejecución: " << time(NULL) - (time_t)timeIni << endl;
        }else if (strcmp(comando,"x") == 0) { 
                // TODO: destruir todas las estructuras e hilos de clientes                
                printf("Chauuuuuuuu!\n");
                break;
        }else{
                printf("Error: comando invalido.\n");
        }
    }
    
    return NULL;
}