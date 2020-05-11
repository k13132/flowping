/*
 * File:   cClient.cpp
 *
 * Copyright (C) 2016: Department of Telecommunication Engineering, FEE, CTU in Prague
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


#include <netdb.h>
#include <cstdlib>
#include <sys/socket.h>
#include <ctime>
#include <sstream>
#include <vector>
#include <cstring>
#include "cClient.h"
#include <future>

using namespace std;

cClient::cClient(cSetup *setup, cStats *stats, cMessageBroker *mbroker) {
    first = true;
    this->setup = setup;
    if (stats) {
        this->stats = (cClientStats *) stats;
    }else{
        this->stats = nullptr;
    }

    if (mbroker) {
        this->mbroker = mbroker;
    }else{
        this->mbroker = nullptr;
    }

    this->s_running = false;
    this->r_running = false;
    //this->gennerator_running = false;
    setup->setStarted(false);
    setup->setStop(false);
    setup->setDone(false);
    this->t1 = (u_int64_t) (setup->getTime_t()*1000000000.0); //convert s to ns
    this->t2 = (u_int64_t) (this-> t1 + setup->getTime_T()*1000000000.0); //convert s to ns
    this->t3 = (u_int64_t) (setup->getTime_R()*1000000000.0); //convert s to ns
    this->base_interval = (u_int64_t) setup->getInterval_i();
    this->min_interval = setup->getMinInterval();
    this->max_interval = setup->getMaxInterval();
    this->bchange = setup->getBchange();
    this->pktBufferReady = false;
    this->senderReady = false;
    this->receiverReady = false;
}

cClient::~cClient() {
    close(this->sock);
}

bool cClient::isSenderReceiverReady() {
    return this->senderReady && this->receiverReady;
}

int cClient::run_packetFactory() {
    bool pkt_created;
    while (!setup->tpReady()) {
        usleep(50000);
    }
    pkt_created = setup->prepNextPacket();
    if (pkt_created) {
        this->pktBufferReady = true;
    }
    while (!setup->isDone() && !setup->isStop()) {
        //std::cout << "pbuffer size: " << setup->getTimedBufferSize()<<std::endl;
        while (pkt_created && setup->getTimedBufferSize() < 96000) {
            pkt_created = setup->prepNextPacket();

            if (setup->isStop()) {
                return 0;
            }
        }
        if (!pkt_created) {
            //std::cout << "Packet not prepared." << std::endl;
            return 0;
        }
        usleep(1);
    }
    //std::cout << "Packet Factory is done." << std::endl;
    return 0;
}

int cClient::run_receiver() {
    struct hostent *hp;
    this->r_running = true;

    // Convert the host name as a dotted-decimal number.
    bzero((void *) &saServer, sizeof (saServer));
    if ((hp = gethostbyname(setup->getHostname().c_str())) == nullptr) {
        perror("host name error");
        exit(1);
    }

    bcopy(hp->h_addr, (char *) &saServer.sin_addr, hp->h_length);

    // Create a UDP/IP datagram socket
    this->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (this->sock < 0) {
        perror("Failed in creating socket");
        exit(1);
    }

    if (setup->useInterface()) {
        setsockopt(this->sock, SOL_SOCKET, SO_BINDTODEVICE, setup->getInterface().c_str(), strlen(setup->getInterface().c_str()));
    }

    // Fill in the address structure for the server
    saServer.sin_family = AF_INET;
    // Port number from command line
    saServer.sin_port = htons(setup->getPort());

    // Synchronization point
    this->receiverReady = true;
    while (not isSenderReceiverReady()){
        usleep(200000);
    }

    unsigned char packet[MAX_PKT_SIZE + 60];
    int nRet;

    int nFromLen;

    nFromLen = sizeof (struct sockaddr);
    clock_gettime(CLOCK_REALTIME, &r_curTv);

    //MAIN RX LOOP  *******************************************
    gen_msg_t * msg;

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(this->sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));

    while (!setup->isDone()) {
        nRet = recvfrom(this->sock, packet, MAX_PKT_SIZE, 0, (struct sockaddr *) &saServer, (socklen_t *) & nFromLen);
        if (nRet < 0) {
            msg = new gen_msg_t;
            msg->type = MSG_KEEP_ALIVE;
            mbroker->push_lp(msg);
            continue;
        }
        msg = new gen_msg_t;
        memcpy(msg,packet, HEADER_LENGTH);
        msg->size = nRet;
        mbroker->push_rx(msg);
    }
    this->r_running = false;
    return 1;
}

int cClient::run_sender() {
    this->s_running = true;
    event_t event;
    struct ping_pkt_t *ping_pkt;
    struct ping_msg_t *ping_msg;
    struct gen_msg_t *gen_msg;
    stringstream ss;
    //Todo RANDOM PACKET CONTENT
    unsigned char packet[MAX_PKT_SIZE + 60] = {0}; //Random FILL will be better
    int nRet;
    //bool show = not setup->silent();

    //initiate output
    gen_msg_t * t = new gen_msg_t;
    t->type = MSG_OUTPUT_INIT;
    mbroker->push_lp(t);


    delta = 0;
    clock_gettime(CLOCK_REALTIME, &sentTv); //FIX initial delta

    ping_pkt = (struct ping_pkt_t*) (packet);
    ping_msg = (struct ping_msg_t*) (packet);

    ping_msg->params = 0;
    ping_pkt->type = CONTROL; //prepare the first packet
    ping_msg->code = CNT_NONE;
    ping_msg->size = MIN_PKT_SIZE;
    if (setup->showTimeStamps()) {
        ping_msg->params = (ping_msg->params | CNT_DPAR);
    }
    if (setup->showBitrate()) {
        ping_msg->params = (ping_msg->params | CNT_ePAR);
    }
    if (setup->showSendBitrate()) {
        ping_msg->params = (ping_msg->params | CNT_EPAR);
    }
    if (setup->wholeFrame()) {
        ping_msg->params = (ping_msg->params | CNT_HPAR);
    }
    if (setup->isAsym()) {
        ping_msg->params = (ping_msg->params | CNT_XPAR);
        ping_msg->size = setup->getFirstPacketSize();
        setup->setPayoadSize(MIN_PKT_SIZE);
    }
    if (setup->toCSV()) {
        ping_msg->params = (ping_msg->params | CNT_CPAR);
    }
    if (setup->toJSON()) {
        ping_msg->params = (ping_msg->params | CNT_JPAR);
    }
    if (setup->getF_Filename().length() && setup->sendFilename()) {
        strcpy(ping_msg->msg, setup->getF_Filename().c_str());
        ping_msg->code = CNT_FNAME;
    } else {
        ping_msg->code = CNT_NOFNAME;
    }
    ping_msg->size = HEADER_LENGTH + strlen(ping_msg->msg);
    int timeout = 0;
    // Synchronization point
    this->senderReady = true;
    while (not isSenderReceiverReady()){
        usleep(200000);
    }
    while (!setup->isStarted()) {
#ifdef DEBUG        
        cerr << "Sending CONTROL Packet - code:" << ping_msg->code << endl;
#endif
        nRet = sendto(this->sock, packet, ping_msg->size, 0, (struct sockaddr *) &saServer, sizeof (struct sockaddr));
        if (nRet < 0) {
            cerr << "Send ERROR\n";
            close(this->sock);
            exit(1);
        }
        usleep(200000); //  5pkt/s
        timeout++;
        if (timeout == 50) { //try contact server fo 10s
            cerr << "Can't connect to server\n";
            exit(1);
        }
    }

    ping_pkt->type = PING; //prepare the first packet
    clock_gettime(CLOCK_REALTIME, &start_ts);
    my_ts.tv_sec += setup->getTime_t();
    ping_pkt->sec = start_ts.tv_sec;
    ping_pkt->nsec = start_ts.tv_nsec;
    u_int16_t payload_size;

    clock_gettime(CLOCK_REALTIME, &refTv);


    timespec ts;
    timed_packet_t tinfo;
    u_int64_t tgTime = 0;

    gen_msg_t *msg;
    while (!pktBufferReady) {
        usleep(200000);
    }
    u_int64_t start_time = NS_TIME(start_ts);
    u_int64_t deadline = setup->getDeadline() * 1000000000L + start_time;
    for (unsigned int i = 1; (i <= setup->getCount() && not setup->isStop()); i++) {
        refTv = sentTv;
        clock_gettime(CLOCK_REALTIME, &sentTv);
        curTv = sentTv;
        if (setup->nextPacket()) {
            tinfo = setup->getNextPacket();
            //Target time
            tgTime = start_time + tinfo.ts;
            // Drop delayed packets (+ 10 ms safe zone);
            if (NS_TIME(curTv) > (tgTime + 10000)) continue;

            if (setup->actWaiting()) {
                while (NS_TIME(curTv) < tgTime) {
                    clock_gettime(CLOCK_REALTIME, &curTv);
                }
            } else {
                ts.tv_sec = tgTime / 1000000000L;
                ts.tv_nsec = tgTime % 1000000000L;
                setup->recordLastDelay(ts);
                delay(ts);
                clock_gettime(CLOCK_REALTIME, &curTv);
            }
            payload_size = tinfo.len;
            ping_pkt->sec = curTv.tv_sec;
            ping_pkt->nsec = curTv.tv_nsec;
            ping_pkt->size = payload_size; //info for server side in AntiAsym mode; //Real size should be obtained as ret size of send and receive
            ping_pkt->seq = i;

            //check deadline
            if (deadline < (curTv.tv_sec * 1000000000L + curTv.tv_nsec)){
                setup->setStop(true);
            }
        } else {
            setup->setStop(true);
            break;
        }

        if (setup->isAntiAsym()) {
            payload_size = MIN_PKT_SIZE;
        }
        //SEND PKT ************************
        nRet = sendto(this->sock, packet, payload_size, 0 , (struct sockaddr *) &saServer, sizeof (struct sockaddr));
        msg = new(gen_msg_t);
        //Todo May not be safe / nRet (packet) size can be lower than gen_msg_t -> but only in case of corrupted packet / socket reading failure
        memcpy(msg,packet, sizeof(gen_msg_t));
        msg->type = MSG_TX_PKT;
        msg->size = nRet;
        mbroker->push_tx(msg);
    }
    if (setup->nextPacket()) {
        std::cerr << "PBuffer not empty." << std::endl;
    }
    ping_pkt->type = CONTROL;
    t = new gen_msg_t;
    t->type = MSG_TIMER_END;
    mbroker->push_lp(t);
    ping_msg->code = CNT_DONE;
    ping_msg->size = MIN_PKT_SIZE;
    timeout = 0;
    sleep(1); //wait for network congestion "partialy" disapear.
    while (!setup->isDone()) {
        //std::cout << "Get server stats..." << std::endl;
        nRet = sendto(this->sock, packet, ping_msg->size, 0, (struct sockaddr *) &saServer, sizeof (struct sockaddr));
        if (nRet < 0) {
            cerr << "Send ERRROR\n";
            close(this->sock);
            exit(1);
        }
        usleep(250000); //  4pkt/s
        timeout++;
        if (timeout == 60) { //15s
            cerr << "Can't get stats from server\n";
            exit(1);
        }
    }
    //std::cout << "sender is done" << std::endl;
    this->s_running = false;
    return 0;
}


void cClient::terminate() {
    setup->setStop(true);
}


void cClient::delay(timespec req) {
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &req, &req);
}

bool cClient::status() {
    return (this->r_running || this->s_running);
}