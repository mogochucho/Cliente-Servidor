/* 
 * File:   WellcomeSocket.cpp
 * Author: matias
 * 
 * Created on 5 de octubre de 2015, 11:07 AM
 */

#include "WellcomeSocket.h"
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>

using namespace std;

WellcomeSocket::WellcomeSocket(int puerto) {
    int descriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if(descriptor == -1){
        throw;
    }
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(puerto);
    if(bind(descriptor, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        close(descriptor);
        throw;
    }
    
    // Asignando atributos
    this->descriptor = descriptor;
}

sockaddr_in* WellcomeSocket::aceptar() {
    sockaddr_in* addr;
    if((addr = (sockaddr_in*) malloc(sizeof(sockaddr_in))) == NULL)
        throw -2;
    socklen_t len_addr = sizeof(*addr);
    while(true) {
        if((recvfrom(this->descriptor, NULL, 0, 0, (struct sockaddr*) addr, &len_addr)) >= 0) {
            return addr;
        }
    }
}

WellcomeSocket::~WellcomeSocket() {
}
