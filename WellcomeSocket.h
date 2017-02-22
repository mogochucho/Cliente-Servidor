/* 
 * File:   WellcomeSocket.h
 * Author: matias
 *
 * Created on 5 de octubre de 2015, 11:07 AM
 */

#ifndef WELLCOMESOCKET_H
#define	WELLCOMESOCKET_H

#include <netinet/in.h>
#include <unistd.h>
#include "Cliente.h"

class WellcomeSocket {
public:
    WellcomeSocket(int puerto);
    sockaddr_in* aceptar();
    virtual ~WellcomeSocket();
private:
    int descriptor;
};

#endif	/* WELLCOMESOCKET_H */

