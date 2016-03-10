/* 
 * File:   server.h
 * 
 * Author: Ondrej Vondrous, KTT, CVUT
 * Email: vondrond@fel.cvut.cz
 * Copyright: Department of Telecommunication Engineering, FEE, CTU in Prague 
 * License: Creative Commons 3.0 BY-NC-SA

 * This file is part of FlowPing.
 *
 *  FlowPing is free software: you can redistribute it and/or modify
 *  it under the terms of the Creative Commons BY-NC-SA License v 3.0.
 *
 *  FlowPing is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY.
 *
 *  You should have received a copy of the Creative Commons 3.0 BY-NC-SA License
 *  along with FlowPing.
 *  If not, see <http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode>. 
 *   
 */



#ifndef SERVER_H
#define	SERVER_H

#include "_types.h"
#include "cSetup.h"
#include <map>


struct t_conn{
    timespec  curTv, refTv;
    u_int64_t pkt_cnt;
    vector <string> msg_store;
    bool C_par;
    bool D_par;
    bool e_par;
    bool E_par;
    bool F_par;
    bool H_par;
    bool X_par;
    bool AX_par;
    bool W_par;
    FILE * fp;
};

//#include "uping.h"

class cServer {
public:
    cServer(cSetup *setup);
    void terminate(void);
    virtual ~cServer();
    int run(void);
private:
    uint64_t conn_id;
    map <uint64_t,t_conn *> connections;
    t_conn * connection;
    cSetup *setup;
    int sock;
    bool stop;
};

#endif	/* SERVER_H */

