/*
 * File:   cServer.cpp
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


#include <cstdlib>
#include <sstream>
#include "cServer.h"
#include "types.h"
#include "cSlotTimer.h"
#include <filesystem>
#include <chrono>

using namespace std;

cServer::cServer(cSetup *setup, cMessageBroker *mbroker, cConnectionBroker *cbroker, cSlotTimer *stimer) {
    this->setup = setup;

    if (mbroker) {
        this->mbroker = mbroker;
    }else{
        this->mbroker = nullptr;
    }

    if (cbroker) {
        this->cbroker = cbroker;
    }else{
        this->cbroker = nullptr;
    }

    if (stimer) {
        this->stimer = stimer;
    }else{
        this->stimer = nullptr;
    }

    this->stop = false;
}

cServer::~cServer() {
    close(this->sock);
}

int cServer::run() {
    struct sockaddr_in6 saServer6, saClient6;
    int addr_len = sizeof saClient6;
    stringstream message;
    stringstream ss;
    int on=1;
    // Create a UDP socket
    if ((this->sock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        perror("Can't open stream socket");
        exit(1);
    }
    //Set socket timeout
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(this->sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0){
        perror("setsockopt(SO_RCVTIMEO) failed");
    }
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    if (setsockopt(this->sock, SOL_SOCKET, SO_SNDTIMEO,&tv,sizeof(tv)) < 0){
        perror("setsockopt(SO_SNDTIMEO) failed");
    }
    if (setsockopt(this->sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0){
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    //some duplicities for better readability
    int snd_BufferSize = setup->getSocketSndBufferSize();
    int rcv_BufferSize = setup->getSocketRcvBufferSize();
    int snd_sockOptSize = sizeof(snd_BufferSize);
    int rcv_sockOptSize = sizeof(rcv_BufferSize);
    setsockopt(this->sock, SOL_SOCKET, SO_SNDBUF,&snd_BufferSize,snd_sockOptSize);
    setsockopt(this->sock, SOL_SOCKET, SO_RCVBUF,&rcv_BufferSize,rcv_sockOptSize);

    // Fill in the address structure
    bzero((char *) & saClient6, sizeof (saClient6));
    bzero((char *) & saServer6, sizeof (saServer6));
    saServer6.sin6_family = AF_INET6;
    saServer6.sin6_addr = in6addr_any; // Auto asssign address and allow connection from IPv4 and IPv6
    saServer6.sin6_port = htons(setup->getPort()); // Set listening port

    // bind to the socket
    int ret_size = bind(this->sock, (struct sockaddr *) & saServer6, sizeof (saServer6));
    if (ret_size < 0) {
        perror("Can't bind to local address");
        exit(1);
    }
    // Show the server name and port number
    //if (setup->debug()) {
    char hostname[200];
    ret_size = gethostname(hostname, sizeof (hostname));
    if (ret_size < 0) {
        perror("gethostname");
        exit(1);
    }

    printf("FlowPing server on %s waiting on port %d\n", hostname, setup->getPort());
    //}

    unsigned char packet[MAX_PAYLOAD_SIZE + HEADER_LENGTH];

    //
    // MAIN LOOP /////////////////////////////////
    //
    conn_t * connection = nullptr;
    stimer->start();
    while (!stop) {
        ret_size = recvfrom(this->sock, packet, MAX_PAYLOAD_SIZE + HEADER_LENGTH, 0, (struct sockaddr *) &saClient6, (socklen_t *) & addr_len);
        gen_msg_t *msg = nullptr;
        gen_msg_t *tmsg = nullptr;
        if (ret_size < 0) {
            tmsg = new gen_msg_t;
            tmsg->type = MSG_KEEP_ALIVE;
            mbroker->push_lp(tmsg);
            continue;
        }
        connection = cbroker->getConn(saClient6, (ping_pkt_t *)packet, setup->isAsym());
        if (connection == nullptr){
            continue;
        }
        connection->size = ret_size;
        clock_gettime(CLOCK_REALTIME, &connection->curTv);
        //connection->refTv = connection->curTv;
        msg = (struct gen_msg_t*) (packet);
        if (msg->type == PING) {
            ((ping_pkt_t *)packet)->server_sec = connection->curTv.tv_sec;
            ((ping_pkt_t *)packet)->server_nsec = connection->curTv.tv_nsec;
            ret_size = sendto(this->sock, packet, connection->ret_size, 0, (struct sockaddr *) &saClient6, addr_len);

            //RX STATS
            tmsg = new(gen_msg_t);
            memcpy(tmsg,packet, sizeof(gen_msg_t));
            tmsg->type = MSG_RX_PKT;
            //tmsg->size = ret_size;
            mbroker->push(tmsg, connection);

            if (ret_size < 0) {
                //Unable to send packet -> no TX stats needed
                continue;
            }
            //TX STATS
            tmsg = new(gen_msg_t);
            memcpy(tmsg,packet, sizeof(gen_msg_t));
            tmsg->type = MSG_TX_PKT;
            tmsg->size = ret_size;
            mbroker->push(tmsg, connection);
        }else{
            //initiate output
            processControlMessage(msg, connection);
            if (!connection->initialized){
                if (msg->type != CNT_TERM){
                    gen_msg_t * t = new gen_msg_t;
                    t->type = MSG_OUTPUT_INIT;
                    mbroker->push(t, connection);
                    connection->initialized = true;
                    stimer->addTimer(connection);
                }
            }
            sendto(this->sock, msg, MIN_PKT_SIZE, 0, (struct sockaddr *) &saClient6, addr_len);
            //clock_gettime(CLOCK_REALTIME, &connection->curTv);
        }
    }
    stimer->stop();
    return 1;
}

void cServer::processControlMessage(gen_msg_t *msg, conn_t * connection){
    if (connection->finished) return;
    gen_msg_t * tmsg = nullptr;
    timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);
    if (msg->type == CONTROL) {
        std::stringstream msg_out;
        msg_out.str("");
        ping_msg_t *ping_msg = (ping_msg_t *) msg;
        std::string path = "";
        //Reduce msg size to header size if no message is included in control PKT
        if (ping_msg->code != CNT_FNAME) ping_msg->size = MIN_PKT_SIZE;
        //std::cout << "CTR MSG COde received: " << (uint16_t) msg->id << " / "<< (uint16_t )ping_msg->code << std::endl;
        switch (ping_msg->code) {
            case CNT_FNAME:
                if (connection->started){
                    if (NS_TDIFF(connection->curTv, tv)<10000000000){
                        break;
                    }
                }
                if (connection->fout.is_open()) {
                    //drop duplicate message in first 10 secs
                    std::cerr << "closing file!" << std::endl;
                    connection->fout.close();
                }
                path = std::string(ping_msg->msg);
                //Only filename is allowed to make it through
                path = path.substr(path.find_last_of("//") + 1);
                //Read time directly from FS works also among multiple FP instances
                //We expect POSIX attributes on FS
                if (std::filesystem::exists(setup->getWorkingDirectory() + "/" + path)){
                    std::filesystem::file_time_type ftime = std::filesystem::last_write_time(setup->getWorkingDirectory() + "/" + path);
                    auto now = std::filesystem::file_time_type::clock::now();
                    uint64_t age = std::chrono::duration_cast<std::chrono::seconds>(now - ftime).count();
                    if (age < 10) {
                        std::cerr << "Trying to open active file, closing connection!" << std::endl;
                        ping_msg->code = CNT_TERM;
                        connection->finished  = true;
                        break;
                    }
                }
                connection->fout.open(setup->getWorkingDirectory() + "/" + path);
                ping_msg->code = CNT_FNAME_OK;
                if (not connection->fout.is_open()) {
                    //Todo wrong place for redirect
                    std::cerr << "Unable to open file, redirecting to STDOUT" << std::endl;
                    ping_msg->code = CNT_OUTPUT_REDIR;
                }else{
                    connection->F_par = true;
                }
                processControlMessage(msg, connection);
                break;

            case CNT_OUTPUT_REDIR:
                //ToDo
                break;

            case CNT_NOFNAME:
                if (connection->started){
                    //drop duplicate message in first 10 secs
                    if (NS_TDIFF(connection->curTv, tv)<10000000000){
                        break;
                    }
                }
                //std::cerr << "CNT NOFNAME" << std::endl;
                if (setup->getFilename().length() && setup->outToFile()) {
                    connection->fout.open(setup->getFilename().c_str());
                    if (not connection->fout.is_open()) {
                        //Todo wrong place for redirect
                        std::cerr << "Unable to open file, redirecting to STDOUT" << std::endl;
                    }
                }
                ping_msg->code = CNT_FNAME_OK;
                processControlMessage(msg, connection);
                break;

            case CNT_DONE:
                //std::cerr << "CNT DONE" << std::endl;
                ping_msg->code = CNT_DONE_OK;
                ping_msg->count = connection->pkt_rx_cnt;
                setup->setExtFilename(string());
                tmsg = new gen_msg_t;
                tmsg->type = MSG_OUTPUT_CLOSE;
                mbroker->push(tmsg, connection);
                processControlMessage(msg, connection);
                break;

            case CNT_DONE_OK:
                msg_out << ".::. Test from " << connection->client_ip << " finished.  ~  " << connection->pkt_tx_cnt << "/" << connection->pkt_rx_cnt << " packets processed." << endl;
                cerr << msg_out.str() << endl;
                stimer->removeTimer(connection->conn_id);
                break;

            case CNT_FNAME_OK:
                msg_out << endl << ".::. Test from " << connection->client_ip << " started. \t\t[";
                setup->setAntiAsym(false);
                // std::cout << (uint16_t )setup->extFilenameLen() << std::endl;
                //if (connection->F_par) std::cout << "F_par" <<std::endl;
                if (setup->extFilenameLen() || connection->F_par) {
                    msg_out << "F";
                }
                if (ping_msg->params & CNT_HPAR) {
                    setup->setHPAR(true);
                    connection->H_par = true;
                    msg_out << "H";

                } else {
                    setup->setHPAR(false);
                }
                if (ping_msg->params & CNT_DPAR) {
                    connection->D_par = true;
                    msg_out << "D";
                } else {
                    connection->D_par = setup->showTimeStamps();
                }
                if (ping_msg->params & CNT_ePAR) {
                    connection->e_par = true;
                    msg_out << "e";
                } else {
                    connection->e_par = setup->showBitrate();
                }

                if (ping_msg->params & CNT_EPAR) {
                    connection->E_par = true;
                    msg_out << "E";
                } else {
                    connection->E_par = setup->showSendBitrate();
                }

                if (ping_msg->params & CNT_LPAR) {
                    setup->setLPAR(true);
                    connection->L_par = true;
                    connection->sample_len = (uint64_t)ping_msg->sample_len_ms * 1000000L;
                    msg_out << "L";
                } else {
                    setup->setLPAR(false);
                }
                if (ping_msg->params & CNT_XPAR) {
                    setup->setXPAR(false);
                    connection->X_par = false;
                    connection->AX_par = true;
                    msg_out << "X";
                    setup->setPayoadSize(ping_msg->size);
                    setup->setAntiAsym(true);
                } else {
                    setup->restoreXPAR();
                }
                if (ping_msg->params & CNT_CPAR) {
                    setup->setCPAR(true);
                    connection->C_par = true;
                    msg_out << "C";
                } else {
                    setup->setCPAR(false);
                }
                if (ping_msg->params & CNT_JPAR) {
                    setup->setJPAR(true);
                    connection->J_par = true;
                    msg_out << "J";
                } else {
                    setup->setJPAR(false);
                }
                msg_out << "]";
                cerr << msg_out.str() << endl;
                connection->started = true;
                break;

        }
    }
};

//we need to alert all clients if there are active connections
void cServer::terminate() {
    ping_msg_t * tmsg = nullptr;
    std::cerr << "Terminate called" << std::endl;
    int16_t ret_size = 0;
    uint16_t counter = setup->getMaxInvitePackets();
    uint16_t delay_corrections;
    conn_t * conn;
    while (cbroker->getConnCount() && counter){
        //std::cerr << "Connection remaining: " << (std::uint16_t) cbroker->getConnCount() << std::endl;
        vector conn_ids = cbroker->getActiveConnIDs();
        delay_corrections = 0;
        for (int conn_id : conn_ids) {
            conn = cbroker->getConnByID(conn_id);
            if (conn->finished) continue; //we do not expect finished connection here, but...
            tmsg = new ping_msg_t;
            tmsg->type = CONTROL;
            tmsg->code = CNT_TERM;
            if (conn->family == AF_INET6 ){
                //std::cerr << "sending MSG_TERM over IPv6" << std::endl;
                ret_size = sendto(this->sock, tmsg, MIN_PKT_SIZE, 0, (struct sockaddr *) &conn->saddr, INET6_ADDRSTRLEN);
            }else{
                //std::cerr << "sending MSG_TERM over IPv4" << std::endl;
                ret_size = sendto(this->sock, tmsg, MIN_PKT_SIZE, 0, (struct sockaddr *) &conn->saddr, INET_ADDRSTRLEN);
            }
            if (ret_size < 0){
                std::cerr << "Unable to send packet" << std::endl;
            }
            usleep(10000);  //10ms delay before next connection is targeted.
            delay_corrections ++;
        }
        if (setup->getInvitePacketRepeatDelay() > delay_corrections * 10000){
            usleep(setup->getInvitePacketRepeatDelay() - delay_corrections * 10000);
        }
        counter--;
    }
    std::cerr << "Stopping services" << std::endl;
    cbroker->stop();
    this->stop = true;
    setup->setDone(true);
    std::cerr << "Server not running." << std::endl;
}



//void cServer::terminate() {
//    std::cerr << "Terminate called" << std::endl;
//    this->stop = true;
//    setup->setDone(true);
//}


