/*
 * File:   cClient.h
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



#ifndef CLIENT_H
#define	CLIENT_H

#include "_types.h"
#include "cSetup.h"
#include "cStats.h"
#include "cSlotTimer.h"
#include "flowping.h"
#include "queue"
#include <netdb.h>
#include <cstdlib>
#include <sys/socket.h>

using namespace std;

class cClient {
public:
    cClient(cSetup *setup, cStats *stats, cMessageBroker *mbroker, cSlotTimer* stimer);
    bool status(void);
    int run_sender(void);
    int run_receiver(void);
    int run_packetFactory(void);
    void terminate(void);
    virtual ~cClient();

private:
    bool first;
    bool r_running,s_running;
    bool pktBufferReady;
    vector <event_t> msg_store;
    vector <event_t> msg_store_snd;
    queue <ping_pkt_t> rcv_queue;
    void delay(struct timespec);

    struct ping_pkt_t *ping_pkt;
    struct ping_msg_t *ping_msg;
    char addrstr[32];
    cMessageBroker *mbroker;
    cSetup *setup;
    cSlotTimer* stimer;
    cClientStats *stats;
    
    bool senderReady;
    bool receiverReady;
    bool isSenderReceiverReady();


    int sock;
    struct sockaddr_in6 saServer6, saClient6;
    struct addrinfo hints, *resAddr=NULL, *ipv4_Addr = NULL, *ipv6_Addr = NULL;
    struct timeval my_ts, rrefTv, refTv2,  tmpTv;
    struct timespec req, rem, refTv, curTv, sentTv, r_curTv, r_refTv, start_ts;
    double r_delta, delta, delta2, delta3;
    double sent_ts, tSent;
    double interval_i, interval_I;
    pthread_barrier_t barr;
    u_int64_t t1, t2, t3;
    double bchange; //interval change - bitrate change
    u_int16_t frame_size;
    u_int64_t base_interval;
    u_int64_t interval, min_interval, max_interval,prev_interval;

};

#endif	/* CLIENT_H */

