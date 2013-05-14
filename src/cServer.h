/* 
 * File:   server.h
 *
 * Author: Ondrej Vondrous
 * Email: vondrous@0xFF.cz
 * 
 * Created on 26. červen 2012, 22:37
 */

#ifndef SERVER_H
#define	SERVER_H

#include "cSetup.h"
#include "_types.h"
#include "uping.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>

class cServer {
public:
    cServer(cSetup *setup);
    void terminate(void);
    virtual ~cServer();
    int run(void);
private:
    cSetup *setup;
    int sock;
    bool stop;
};

#endif	/* SERVER_H */

