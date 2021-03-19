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
#include "_types.h"

using namespace std;

cServer::cServer(cSetup *setup, cStats *stats, cMessageBroker *mbroker) {
    this->mbroker = mbroker;
    this->setup = setup;
    if (stats){
        this->stats = (cServerStats *) stats;
    }else{
        this->stats = nullptr;
    }

    if (mbroker) {
        this->mbroker = mbroker;
    }else{
        this->mbroker = nullptr;
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
    if (setsockopt(this->sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0){
        perror("setsockopt(SO_REUSEADDR) failed");
    }

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

    printf("\nFlowPing server on %s waiting on port %d\n", hostname, setup->getPort());

    //}

    unsigned char packet[MAX_PKT_SIZE + 60];
    // MAIN LOOP /////////////////////////////////
    while (!stop) {
        ret_size = recvfrom(this->sock, packet, MAX_PKT_SIZE, 0, (struct sockaddr *) &saClient6, (socklen_t *) & addr_len);
        gen_msg_t *msg = nullptr;
        gen_msg_t *tmsg = nullptr;
        if (ret_size < 0) {
            tmsg = new gen_msg_t;
            tmsg->type = MSG_KEEP_ALIVE;
            mbroker->push_lp(tmsg);
            continue;
        }
        //connection = getConnection6(saClient6);
        connection = getConnectionFID(saClient6, (ping_pkt_t *)packet);
        connection->refTv = connection->curTv;
        clock_gettime(CLOCK_REALTIME, &connection->curTv);
        msg = (struct gen_msg_t*) (packet);
        if (msg->type == PING) {
            if (setup->isAsym(connection->X_par)) ret_size = MIN_PKT_SIZE;
            if (setup->isAntiAsym(connection->AX_par)) {
                if (setup->wholeFrame(connection->H_par)) {
                    ret_size = msg->size - 42;
                } else {
                    ret_size = msg->size;
                }
            }
            sendto(this->sock, packet, ret_size, 0, (struct sockaddr *) &saClient6, addr_len);
            connection->pkt_cnt++;
            //ToDo Nefunguje na OpenWrt, jinde OK //dojde k zaplnněí fronty - nějak nefunguje .pop
            //tmsg = new gen_msg_t;
            //memcpy(tmsg,packet, sizeof(gen_msg_t));
            //mbroker->push(connection, tmsg);
        }else{
            processCMessage(msg, connection);
            ret_size = msg->size;
            sendto(this->sock, msg, ret_size, 0, (struct sockaddr *) &saClient6, addr_len);
            clock_gettime(CLOCK_REALTIME, &connection->curTv);
        }
    }
    return 1;
}

void cServer::processCMessage(gen_msg_t *msg, t_conn * connection){

    //u_int64_t ts;
    u_int64_t conn = connection->conn_id;
    //ToDo
    //stats->connInit(conn_id, string(client_ip), saClient.sin_port);

    if (msg->type == CONTROL) {
        //std::cerr << "CNT MSG" << std::endl;
        //Todo modify structure !!!! packet data not present - ONLY header was copied
        ping_msg_t *ping_msg = (ping_msg_t *) msg;
        ping_msg->size = MIN_PKT_SIZE;
        switch (ping_msg->code) {
            case CNT_FNAME:
                //std::cerr << "CNT FNAME" << std::endl;
                if (connection->fout.is_open()) {
                    connection->fout.close();
                }
                connection->fout.open(setup->getFilename().c_str());
                ping_msg->code = CNT_FNAME_OK;
                if (not connection->fout.is_open()) {
                    //Todo wrong place for redirect
                    std::cerr << "Unable to open file, redirecting to STDOUT" << std::endl;
                    ping_msg->code = CNT_OUTPUT_REDIR;
                }
                processCMessage(msg,connection);
                break;

            case CNT_NOFNAME:
                //std::cerr << "CNT NOFNAME" << std::endl;
                if (setup->getFilename().length() && setup->outToFile()) {
                    connection->fout.open(setup->getFilename().c_str());
                    if (not connection->fout.is_open()) {
                        //Todo wrong place for redirect
                        std::cerr << "Unable to open file, redirecting to STDOUT" << std::endl;
                    }
                }
                ping_msg->code = CNT_FNAME_OK;
                processCMessage(msg,connection);
                break;

            case CNT_DONE:
                //std::cerr << "CNT DONE" << std::endl;
                ping_msg->code = CNT_DONE_OK;
                ping_msg->count = connection->pkt_cnt;
                setup->setExtFilename(string());
                processCMessage(msg,connection);
                break;

            case CNT_DONE_OK:
                //std::cerr << "CNT DONE_OK" << std::endl;
                if (connection->fout.is_open()) {
                    connection->fout.close();
                }
                cerr << ".::. Test from " << connection->client_ip << " finished.  ~  " << connection->pkt_cnt << " packets processed." << endl;
                //cerr << connection << std::endl;
                //delete connection;
                connections.erase(conn);
                break;

            case CNT_FNAME_OK:
                //std::cerr << "CNT FNAME_OK" << std::endl;
                std::stringstream msg_out;
                msg_out.str("");
                msg_out << endl << ".::. Test from " << connection->client_ip << " started. \t\t[";
                setup->setAntiAsym(false);
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

                if (ping_msg->params & CNT_WPAR) {
                    setup->setWPAR(true);
                    connection->W_par = true;
                    msg_out << "W";
                } else {
                    setup->setWPAR(false);
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
                break;

        }
    }
};

void cServer::terminate() {
    std::cerr << "Terminate called" << std::endl;
    this->stop = true;
    setup->setDone(true);
}

string cServer::stripFFFF(string str) {
    if (str.find("::ffff:",0,7) == 0) return str.substr(7);
    return str;
}


t_conn *  cServer::getConnection6(sockaddr_in6 saddr) {
    u_int64_t conn_id = saddr.sin6_addr.s6_addr[0] * (u_int64_t)saddr.sin6_port;
    if (this->connections.count(conn_id) == 1) {
        connection = this->connections.at(conn_id);
    } else {
        char addr[INET6_ADDRSTRLEN];
        connection = new t_conn;
        connection->ip = saddr.sin6_addr;
        connection->port = saddr.sin6_port;
        connection->conn_id = conn_id;
        inet_ntop(AF_INET6, &saddr.sin6_addr, addr, INET6_ADDRSTRLEN);
        connection->client_ip = stripFFFF(string(addr));
        connection->pkt_cnt = 0;
        connection->refTv.tv_sec = 0;
        connection->refTv.tv_nsec = 0;
        connection->curTv.tv_sec = 0;
        connection->curTv.tv_nsec = 0;
        connection->C_par = false;
        connection->D_par = false;
        connection->e_par = false;
        connection->E_par = false;
        connection->H_par = false;
        connection->J_par = false;
        connection->W_par = false;
        connection->X_par = setup->isAsym();
        connection->AX_par = false;
        this->connections[conn_id] = connection;
    }
    return connection;
}

t_conn *  cServer::getConnectionFID(sockaddr_in6 saddr, ping_pkt_t *pkt) {
    //u_int64_t conn_id = saddr.sin6_addr.s6_addr[0] * (u_int64_t)saddr.sin6_port;
    u_int64_t conn_id = (uint64_t) pkt->flow_id;
    if (this->connections.count(conn_id) == 1) {
        connection = this->connections.at(conn_id);
    } else {
        char addr[INET6_ADDRSTRLEN];
        connection = new t_conn;
        connection->ip = saddr.sin6_addr;
        connection->port = saddr.sin6_port;
        connection->conn_id = conn_id;
        inet_ntop(AF_INET6, &saddr.sin6_addr, addr, INET6_ADDRSTRLEN);
        connection->client_ip = stripFFFF(string(addr));
        connection->pkt_cnt = 0;
        connection->refTv.tv_sec = 0;
        connection->refTv.tv_nsec = 0;
        connection->curTv.tv_sec = 0;
        connection->curTv.tv_nsec = 0;
        connection->C_par = false;
        connection->D_par = false;
        connection->e_par = false;
        connection->E_par = false;
        connection->H_par = false;
        connection->J_par = false;
        connection->W_par = false;
        connection->X_par = setup->isAsym();
        connection->AX_par = false;
        this->connections[conn_id] = connection;
    }
    return connection;
}