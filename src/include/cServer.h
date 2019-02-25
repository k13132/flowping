/*
 * File:   cServer.h
 *
 * Copyright (C) 2017: Department of Telecommunication Engineering, FEE, CTU in Prague
 *
 * This file is part of FlowPing.
 *
 * FlowPing is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * FlowPing is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 *
 * You should have received a copy of the GNU General Public License
 * along with FlowPing.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ondrej Vondrous
 *         Department of Telecommunication Engineering
 *         FEE, Czech Technical University in Prague
 *         ondrej.vondrous@fel.cvut.cz
 */



#ifndef SERVER_H
#define	SERVER_H

#include "_types.h"
#include "cSetup.h"
#include "cStats.h"
#include <map>

using namespace std;

struct t_conn{
    timespec  curTv, refTv;
    u_int64_t pkt_cnt;
    vector <string> msg_store;
    bool C_par;
    bool J_par;
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
    cServer(cSetup *setup, cStats *stats, cMessageBroker * mbroker);
    void terminate(void);
    virtual ~cServer();
    int run(void);
private:
    uint64_t conn_id;
    map <u_int64_t, t_conn *> connections;
    t_conn * connection;
    cMessageBroker *mbroker;
    cServerStats *stats;
    cSetup *setup;
    int sock;
    bool stop;
};

#endif	/* SERVER_H */

