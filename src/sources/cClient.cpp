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



#include <ctime>
#include <sstream>
#include <vector>
#include <cstring>
#include "cClient.h"
#include <future>

using namespace std;


unsigned short crc16(const unsigned char* data_p, unsigned char length){
    unsigned char x;
    unsigned short crc = 0xFFFF;

    while (length--){
        x = crc >> 8 ^ *data_p++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
    }
    return crc;
}

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
    int rc;

    memset(&hints, 0x00, sizeof(hints));
    memset(&saServer6, 0x00, sizeof(saServer6));
    //= htons(setup->getPort());
    //hints.ai_flags    = AI_CANONNAME;
    hints.ai_flags    = AI_NUMERICSERV;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    rc = inet_pton(AF_INET, setup->getHostname().c_str(), &saServer6);
    if (rc == 1)    /* valid IPv4 text address? */
    {
        hints.ai_family = AF_INET;
        hints.ai_flags |= AI_NUMERICHOST;
    }
    else
    {
        rc = inet_pton(AF_INET6, setup->getHostname().c_str(), &saServer6);
        if (rc == 1) /* valid IPv6 text address? */
        {
            hints.ai_family = AF_INET6;
            hints.ai_flags |= AI_NUMERICHOST;
        }
    }

    rc = getaddrinfo(setup->getHostname().c_str(), std::to_string(setup->getPort()).c_str(), &hints, &resAddr);
    if (rc != 0)
    {
        printf("Host not found --> %s\n", gai_strerror(rc));
        if (rc == EAI_SYSTEM){
            perror("getaddrinfo() failed");
        }else{
            perror("host name error");
        }
        exit(1);
    }

    //IPv4 vs. IPv6 preference if both types available.
    while(resAddr){
        if (resAddr->ai_family == AF_INET){
            tmpAddr = resAddr;
            //char str[INET6_ADDRSTRLEN];
            //inet_ntop(AF_INET, (in_addr *)resAddr->ai_addr->sa_data, str, INET6_ADDRSTRLEN);
            //inet_pton(AF_INET6, str, &saServer6);
            if (!setup->isIPv6Prefered()){
                break;
            }
        }
        if (resAddr->ai_family == AF_INET6){
            tmpAddr = resAddr;
            if (setup->isIPv6Prefered()){
                break;
            }
        }
        resAddr = resAddr->ai_next;
    }
    resAddr = tmpAddr;
    setup->setAddrFamily(resAddr->ai_family);
    // Create a UDP/IP datagram socket
    this->sock = socket(resAddr->ai_family, resAddr->ai_socktype, resAddr->ai_protocol);
    //this->sock = socket(resAddr->ai_family, SOCK_DGRAM, IPPROTO_UDP);
    if (this->sock < 0) {
        perror("Failed in creating socket");
        exit(1);
    }
    if (setup->useInterface()) {
        setsockopt(this->sock, SOL_SOCKET, SO_BINDTODEVICE, setup->getInterface().c_str(), strlen(setup->getInterface().c_str()));
    }

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


    this->r_running = true;
    while (!setup->isDone()) {
        nRet = recvfrom(this->sock, packet, MAX_PKT_SIZE, 0, resAddr->ai_addr, &resAddr->ai_addrlen);
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
    stringstream ss;
    int nRet;

    unsigned char packet[MAX_PKT_SIZE + 60] = {0};

    //initialize random number generator
    uint64_t milliseconds_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::srand((unsigned)milliseconds_since_epoch);

    for (int i = 0; i< sizeof packet; i++){
        packet[i] = (uint8_t)(rand() % 256);
    }

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

    //initiate output
    gen_msg_t * t = new gen_msg_t;
    t->type = MSG_OUTPUT_INIT;
    mbroker->push_lp(t);

    while (!setup->isStarted()) {
#ifdef DEBUG        
        cerr << "Sending CONTROL Packet - code:" << ping_msg->code << endl;
#endif
        nRet = sendto(this->sock, packet, ping_msg->size, 0, resAddr->ai_addr, resAddr->ai_addrlen);
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
    ping_pkt->flow_id = (uint16_t)(rand() % 65536);
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
    unsigned int i;
    for (i = 1; (i <= setup->getCount() && not setup->isStop());) {
        refTv = sentTv;
        clock_gettime(CLOCK_REALTIME, &sentTv);
        curTv = sentTv;
        if (setup->nextPacket()) {
            tinfo = setup->getNextPacket();
            //Target time
            tgTime = start_time + tinfo.ts;
            // Drop delayed packets (+ 250 ms safe zone);
            if (NS_TIME(curTv) > (tgTime + 250000)) continue;
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
        nRet = sendto(this->sock, packet, payload_size, 0, resAddr->ai_addr, resAddr->ai_addrlen);
        if (nRet < 0) {
            continue;
        }
        msg = new(gen_msg_t);
        memcpy(msg,packet, sizeof(gen_msg_t));
        msg->type = MSG_TX_PKT;
        msg->size = nRet;
        mbroker->push_tx(msg);
        //Increment here - packet can be timeouted
        i++;
    }
    if (setup->nextPacket()) {
        //std::cerr << "PBuffer not empty." << std::endl;
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
        nRet = sendto(this->sock, packet, ping_msg->size, 0, resAddr->ai_addr, resAddr->ai_addrlen);
        if (nRet < 0) {
            cerr << "Send ERRROR\n";
            close(this->sock);
            exit(1);
        }
        //cerr << "send bytes: " << nRet << std::endl;
        usleep(250000); //  4pkt/s
        timeout++;
        if (timeout == 60) { //15s
            //close JSON
            t = new gen_msg_t;
            t->type = MSG_OUTPUT_CLOSE;
            mbroker->push_lp(t);
            cerr << "Can't get stats from server\n";
            usleep(500000);
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