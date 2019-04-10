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
    this->rtt_avg = -1;
    this->rtt_min = -1;
    this->rtt_max = -1;
    this->pkt_rcvd = 0;
    this->pkt_sent = 0;
    this->last_seq_rcv = 0;
    this->ooo_cnt = 0;
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
    if (!setup->useTimedBuffer()) {
        pkt_created = setup->prepNextPacket();
        if (pkt_created) {
            this->pktBufferReady = true;
        }
        while (!setup->isDone() && !setup->isStop()) {
            //std::cout << "pbuffer size: " << setup->getTimedBufferSize()<<std::endl;
            while (pkt_created && setup->getTimedBufferSize() < 64000) {
                pkt_created = setup->prepNextPacket();

                if (setup->isStop()) {
                    return 0;
                }
            }
            if (!pkt_created) {
                std::cout << "Packet not prepared." << std::endl;
                return 0;
            }
            usleep(setup->getTimedBufferDelay() / 25000);
        }
        std::cout << "Packet Factory is done." << std::endl;
    } else {
        while (setup->prepNextPacket());
        if (setup->getTimedBufferSize()) {
            this->pktBufferReady = true;
        }
        msg_store.reserve(setup->getTimedBufferSize() + 10);
        msg_store_snd.reserve(setup->getTimedBufferSize() + 10);
        //cerr << "Allocating message storage for: " << setup->getTimedBufferSize() + 10 << " itemns" << endl;
    }
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
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    setsockopt(this->sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));

    while (!setup->isDone()) {
        nRet = recvfrom(this->sock, packet, MAX_PKT_SIZE, 0, (struct sockaddr *) &saServer, (socklen_t *) & nFromLen);
        if (nRet < 0) {
            usleep(1000);
            continue;
        }
        msg = new(gen_msg_t);
        //std::cout << "RX MSG create at: " << msg << std::endl;
        memcpy(msg,packet, HEADER_LENGTH);
        if (msg->type == MSG_RX_CNT){
            std::cout << "msg type/code: " << (u_int16_t)msg->type << "/" << (u_int16_t)((ping_msg_t *)msg)->code << std::endl;
        }
        mbroker->push_rx(msg);
    }
    std::cout << "receiver is done" << std::endl;
    this->r_running = false;
    this->report();
    return 1;
}

int cClient::run_sender() {
    this->s_running = true;
    event_t event;
    struct ping_pkt_t *ping_pkt;
    struct ping_msg_t *ping_msg;
    stringstream ss;
    unsigned char packet[MAX_PKT_SIZE + 60] = {0}; //Random FILL will be better
    int nRet;
    int pipe_handle;
    pipe_handle = 0; //warning elimination
    bool show = not setup->silent();

    delta = 0;
    clock_gettime(CLOCK_REALTIME, &sentTv); //FIX initial delta

    if (setup->getFilename().length() && setup->outToFile()) {
        fp = fopen(setup->getFilename().c_str(), "w+"); //RW - overwrite file
        fout.open(setup->getFilename().c_str());
        output = &fout;
        if (fp == nullptr) {
            perror("Unable to open file, redirecting to STDOUT");
            fp = stdout;
        } else {
            if (setup->self_check() == SETUP_CHCK_VER) *output << setup->get_version().c_str();
        }
    } else {
        fp = setup->getFP();
        output = setup->getOutput();
    }
    ping_pkt = (struct ping_pkt_t*) (packet);
    ping_msg = (struct ping_msg_t*) (packet);

    ping_msg->params = 0;
    ping_pkt->type = CONTROL; //prepare the first packet
    ping_msg->code = CNT_NONE;
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
    if (setup->useTimedBuffer()) {
        ping_msg->params = (ping_msg->params | CNT_WPAR);
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
    unsigned int pk_len = HEADER_LENGTH + strlen(ping_msg->msg);
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
        nRet = sendto(this->sock, packet, pk_len, 0, (struct sockaddr *) &saServer, sizeof (struct sockaddr));
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

    int pipe_cnt = 0;
    clock_gettime(CLOCK_REALTIME, &refTv);


    timespec ts;
    timed_packet_t tinfo;
    u_int64_t tgTime = 0;

    gen_msg_t *msg;
    while (!pktBufferReady) {
        usleep(200000);
    }
    for (unsigned int i = 1; i <= setup->getCount(); i++) {
        //deadline reached [ -w ];
        if (setup->isStop()) {
            break;
        }
        refTv = sentTv;
        clock_gettime(CLOCK_REALTIME, &sentTv);
        curTv = sentTv;
        if (setup->nextPacket()) {
            tinfo = setup->getNextPacket();
            payload_size = tinfo.len - HEADER_LENGTH;
            ping_pkt->sec = curTv.tv_sec;
            ping_pkt->nsec = curTv.tv_nsec;
            ping_pkt->size = payload_size; //info for server side in AntiAsym mode; //Real size should be obtained as ret size of send and receive
            ping_pkt->seq = i;
            //ToDo chceck if it is possible to compute this in cSetup
            if (setup->isAntiAsym()) {
                payload_size = 0; //No reading from pipe
            }
            if (setup->frameSize()) {
                payload_size -= 42; //todo check negative size of payload.

            }
            //Target time
            tgTime = (uint64_t) ((uint64_t) start_ts.tv_nsec + ((uint64_t) start_ts.tv_sec) * 1000000000)+(tinfo.nsec + tinfo.sec * 1000000000);
            if (setup->actWaiting()) {
                while (((uint64_t) curTv.tv_nsec + ((uint64_t) curTv.tv_sec) * 1000000000L) < tgTime) {
                    clock_gettime(CLOCK_REALTIME, &curTv);
                }
            } else {
                ts.tv_sec = tgTime / 1000000000L;
                ts.tv_nsec = tgTime % 1000000000L;
                setup->recordLastDelay(ts);
                delay(ts);
                clock_gettime(CLOCK_REALTIME, &curTv);
            }
        } else {
            setup->setStop(true);
            break;
        }

        //SEND PKT ************************
        nRet = sendto(this->sock, packet, HEADER_LENGTH + payload_size, 0 , (struct sockaddr *) &saServer, sizeof (struct sockaddr));
        msg = new(gen_msg_t);
        //std::cout << "TX MSG create at: " << msg << std::endl;
        memcpy(msg,packet, HEADER_LENGTH);
        msg->type = MSG_TX_PKT;
        mbroker->push_tx(msg);
        //Todo Remove to improve performance?
//        if (nRet < 0) {
//            cerr << "Packet size:" << HEADER_LENGTH + payload_size << endl;
//            close(this->sock);
//            exit(1);
//        }
        if (setup->frameSize()) nRet += 42;
#ifndef _NOSTATS
        //stats->pktSent(curTv, nRet, ping_pkt->seq);
#endif
    }
    ping_pkt->type = CONTROL;
    ping_msg->code = CNT_DONE;
    timeout = 0;
    //sleep(1); //wait for network congestion "partialy" disapear.
    while (!setup->isDone()) {
        std::cout << "trying to shutdown generator" << std::endl;
        nRet = sendto(this->sock, packet, pk_len, 0, (struct sockaddr *) &saServer, sizeof (struct sockaddr));
        if (nRet < 0) {
            cerr << "Send ERRROR\n";
            close(this->sock);
            exit(1);
        }
        usleep(250000); //  4pkt/s
        timeout++;
        if (timeout == 40) { //10s
            cerr << "Can't get stats from server\n";
            exit(1);
        }
    }
    std::cout << "sender is done" << std::endl;
    this->s_running = false;
    return 0;
}

void cClient::report() {
#ifndef _NOSTATS
    //fprintf(fp, "%s", stats->getReport().c_str());
    //*output << stats->getReport().c_str();
#endif   
    if (setup->toJSON()) {
        //fprintf(fp, "%s", "}\n");
        *output << "}"<<std::endl;
    }
    fclose(fp);
}

void cClient::terminate() {
    setup->setStop(true);
}

u_int16_t cClient::getPacketSize() {
    struct timespec cur_ts;
    u_int16_t payload_size = setup->getPacketSize();
    if (setup->pkSizeChange()) {
        clock_gettime(CLOCK_REALTIME, &cur_ts);
        time = ((cur_ts.tv_sec * 1000000000 + cur_ts.tv_nsec)-(this->start_ts.tv_sec * 1000000000 + this->start_ts.tv_nsec)) % this->t2;
        if (time >= this->t1) {
            u_int16_t tmp = (u_int16_t) (setup->getSchange() * (time - this->t1));
            payload_size = MIN_PKT_SIZE + tmp;
            //cout << "* Payload size: " << payload_size << endl;
            if (payload_size > MAX_PKT_SIZE) payload_size = MAX_PKT_SIZE;
            if (payload_size < MIN_PKT_SIZE) payload_size = MIN_PKT_SIZE;
            return payload_size;
        }
    }
    return payload_size;
}

void cClient::delay(timespec req) {
    //int e = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &req, &req);
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &req, &req);
}

bool cClient::status() {
    return (this->r_running || this->s_running);
}