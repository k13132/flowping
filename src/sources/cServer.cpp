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


#include <stdlib.h>
#include <sstream>
#include "cServer.h"
#include "_types.h"

cServer::cServer(cSetup *setup, cStats *stats) {
    this->setup = setup;
    if (stats){
        this->stats = (cServerStats *) stats;
    }
    this->stop = false;

    //ToDo: zjistit, zdqa je to potreba a zda to ma nejaky prinos    
    /*if (setup->useTimedBuffer()) {
        this->msg_store.resize(10000000);
    }
     */
}

cServer::~cServer() {
    close(this->sock);
}

int cServer::run() {
    struct sockaddr_in saServer;
    bool show = not setup->silent();
    char msg[400] = "";
    char filename[256] = "";
    stringstream message;
    stringstream ss;

    // Create a UDP socket
    if ((this->sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Can't open stream socket");
        exit(1);
    }
    // Fill in the address structure
    bzero((char *) & saServer, sizeof (saServer));
    saServer.sin_family = AF_INET;
    saServer.sin_addr.s_addr = htonl(INADDR_ANY); // Auto asssign address
    saServer.sin_port = htons(setup->getPort()); // Set listening port

    // bind to the socket
    int ret_size;
    int rec_size;
    rec_size = ret_size = bind(this->sock, (struct sockaddr *) & saServer, sizeof (saServer));

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

    int addr_len;
    addr_len = sizeof (sockaddr);
    struct ping_pkt_t *ping_pkt;
    struct ping_msg_t *ping_msg;
    unsigned char packet[MAX_PKT_SIZE + 60];
    struct sockaddr_in saClient;
    int ip;
    char client_ip[INET_ADDRSTRLEN];

    // Wait for data
    while (!stop) {
        rec_size = ret_size = recvfrom(this->sock, packet, MAX_PKT_SIZE, 0, (struct sockaddr *) &saClient, (socklen_t *) & addr_len);
        ip = (saClient.sin_addr.s_addr);
        conn_id = setup->getConnectionID(ip, saClient.sin_port);
        if (connections.count(conn_id) == 1) {
            connection = connections.at(conn_id);
        } else {
            connection = new t_conn;
            connection->fp = setup->getFP();
            connection->pkt_cnt = 0;
            connection->refTv.tv_sec = 0;
            connection->refTv.tv_nsec = 0;
            connection->curTv.tv_sec = 0;
            connection->curTv.tv_nsec = 0;
            connection->C_par = false;
            connection->D_par = false;
            connection->e_par = false;
            connection->E_par = false;
            connection->F_par = false;
            connection->H_par = false;
            connection->J_par = false;
            connection->W_par = false;
            connection->X_par = setup->isAsym();
            connection->AX_par = false;
            connections[conn_id] = connection;
        }
        connection->refTv = connection->curTv;
        clock_gettime(CLOCK_REALTIME, &connection->curTv);
        ping_pkt = (struct ping_pkt_t*) (packet);
        if (ping_pkt->type == PING) {
            if (setup->isAsym(connection->X_par)) ret_size = MIN_PKT_SIZE;
            if (setup->isAntiAsym(connection->AX_par)) {
                if (setup->wholeFrame(connection->H_par)) {
                    ret_size = ping_pkt->size - 42 + HEADER_LENGTH;
                } else {
                    ret_size = ping_pkt->size + HEADER_LENGTH;
                }
                //cout << ret_size << endl;
            }
            sendto(this->sock, packet, ret_size, 0, (struct sockaddr *) &saClient, addr_len);

            //Update stats
            //cout << ping_pkt->size<<endl;
            inet_ntop(AF_INET, &ip, client_ip, INET_ADDRSTRLEN);
#ifndef _NOSTATS
            stats->pktReceived(conn_id, connection->curTv, rec_size, ping_pkt->seq, string(client_ip), saClient.sin_port);
            stats->pktSent(conn_id, connection->curTv, ret_size, ping_pkt->seq, string(client_ip), saClient.sin_port);
#endif            
            if (show) {

                double delta = ((double) (connection->curTv.tv_sec - connection->refTv.tv_sec)*1000.0 + (double) (connection->curTv.tv_nsec - connection->refTv.tv_nsec) / 1000000.0);
                ss.str("");
                if (setup->toJSON(connection->J_par)) {
                    ss << "{";
                }
                if (setup->showTimeStamps(connection->D_par)) {
                    if (setup->toCSV(connection->C_par)) {
                        ss << connection->curTv.tv_sec << ".";
                        ss.fill('0');
                        ss.width(9);
                        ss << connection->curTv.tv_nsec;
                        ss << ";";
                    }
                    if (setup->toJSON(connection->J_par)) {
                        ss << "\"ts\":" << connection->curTv.tv_sec << ".";
                        ss.fill('0');
                        ss.width(9);
                        ss << connection->curTv.tv_nsec;
                        ss << ",";
                    }
                    if (!setup->toCSV(connection->C_par)&&!setup->toJSON(connection->J_par)) {
                        ss << "[" << connection->curTv.tv_sec << ".";
                        ss.fill('0');
                        ss.width(9);
                        ss << connection->curTv.tv_nsec;
                        ss << "] ";
                    }
                } else {
                    if (setup->toCSV(connection->C_par)) {
                        ss << ";";
                    }
                }
                if (setup->wholeFrame(connection->H_par)) {
                    rec_size += 42;
                    ret_size += 42;
                }

                if (setup->toCSV(connection->C_par)) {
                    ss << rec_size << ";" << client_ip << ";" << ping_pkt->seq << ";xx;";
                    ss.setf(ios_base::right, ios_base::adjustfield);
                    ss.setf(ios_base::fixed, ios_base::floatfield);
                    ss.precision(3);
                    ss << delta << ";";
                }
                if (setup->toJSON(connection->J_par)) {
                    ss << "\"size\":" << rec_size << ",\"remote\":\"" << client_ip << "\",\"seq\":" << ping_pkt->seq << ",";
                    ss.setf(ios_base::right, ios_base::adjustfield);
                    ss.setf(ios_base::fixed, ios_base::floatfield);
                    ss.precision(3);
                    ss << "\"delta\":" << delta;
                }
                if (!setup->toCSV(connection->C_par)&&!setup->toJSON(connection->J_par)) {

                    ss << rec_size << " bytes from " << client_ip << ": req=" << ping_pkt->seq << " ttl=xx ";
                    ss.setf(ios_base::right, ios_base::adjustfield);
                    ss.setf(ios_base::fixed, ios_base::floatfield);
                    ss.precision(3);
                    ss << "delta=" << delta << " ms";
                }
                if (setup->showBitrate(connection->e_par)) {
                    if (setup->toCSV(connection->C_par)) {
                        ss.setf(ios_base::right, ios_base::adjustfield);
                        ss.setf(ios_base::fixed, ios_base::floatfield);
                        ss.precision(2);
                        ss << (1000 / delta) * rec_size * 8 / 1000 << ";";
                    }
                    if (setup->toJSON(connection->J_par)) {
                        ss.setf(ios_base::right, ios_base::adjustfield);
                        ss.setf(ios_base::fixed, ios_base::floatfield);
                        ss.precision(2);
                        ss << ",\"rx_bitrate\":" << (1000 / delta) * rec_size * 8 / 1000;
                    }
                    if (!setup->toCSV(connection->C_par)&&!setup->toJSON(connection->J_par)) {
                        ss.setf(ios_base::right, ios_base::adjustfield);
                        ss.setf(ios_base::fixed, ios_base::floatfield);
                        ss.precision(2);
                        ss << " rx_rate=" << (1000 / delta) * rec_size * 8 / 1000 << " kbps";
                    }
                } else {
                    if (setup->toCSV(connection->C_par)) {
                        ss << ";";
                    }
                }
                if (setup->showSendBitrate(connection->E_par)) {
                    if (setup->toCSV(connection->C_par)) {
                        ss.setf(ios_base::right, ios_base::adjustfield);
                        ss.setf(ios_base::fixed, ios_base::floatfield);
                        ss.precision(2);
                        ss << (1000 / delta) * ret_size * 8 / 1000 << ";";
                    }
                    if (setup->toJSON(connection->J_par)) {
                        ss.setf(ios_base::right, ios_base::adjustfield);
                        ss.setf(ios_base::fixed, ios_base::floatfield);
                        ss.precision(2);
                        ss << ",\"tx_bitrate\":" << (1000 / delta) * ret_size * 8 / 1000;
                    }
                    if (!setup->toCSV(connection->C_par)&&!setup->toJSON(connection->J_par)) {
                        ss.setf(ios_base::right, ios_base::adjustfield);
                        ss.setf(ios_base::fixed, ios_base::floatfield);
                        ss.precision(2);
                        ss << " tx_rate=" << (1000 / delta) * ret_size * 8 / 1000 << " kbit/s";
                    }
                    ss << msg;
                } else {
                    if (setup->toCSV(connection->C_par)) {
                        ss << ";";
                    }
                }
                if (setup->toJSON(connection->J_par)) {
                    ss << "}";
                }
                ss << endl;
                if (setup->useTimedBuffer(connection->W_par)) {
                    connection->msg_store.push_back(ss.str());
                } else {
                    fprintf(connection->fp, "%s", ss.str().c_str());

                }
            }
            connection->pkt_cnt++;
        }
        if (ping_pkt->type == CONTROL) {

            inet_ntop(AF_INET, &ip, client_ip, INET_ADDRSTRLEN);
            ping_msg = (struct ping_msg_t*) (packet);
#ifdef DEBUG
            if (setup->debug()) cerr << "Control packet received! code:" << (int) ping_msg->code << endl;
#endif
#ifndef _NOSTATS
            stats->connInit(conn_id, string(client_ip), saClient.sin_port);
#endif
            if (ping_msg->code == CNT_FNAME) {
                if (connection->fp != NULL) {
                    if (connection->fp != stdout) {
                        fclose(connection->fp);
                    }
                }
                connection->fp = fopen(ping_msg->msg, "w+"); //RW - overwrite file
                cerr << ping_msg->msg << endl;
                ping_msg->code = CNT_FNAME_OK;
                if (connection->fp == NULL) {
                    perror("Unable to open file, redirecting to STDOUT");
                    connection->fp = stdout;
                    ping_msg->code = CNT_OUTPUT_REDIR;
                } else {
                    //setup->setExtFilename((string) ping_msg->msg);
                    if (setup->self_check() == SETUP_CHCK_VER) fprintf(connection->fp, "%s", setup->get_version().c_str());
                }
            }
            if (ping_msg->code == CNT_NOFNAME) {
                if (setup->getFilename().length() && setup->outToFile()) {
                    //                    if (connection->fp!= NULL) {
                    //                        if (connection->fp != stdout) {
                    //                            fclose(connection->fp);
                    //                        }
                    //                    }
                    ss.str("");
                    ss << setup->getFilename().c_str() << "_" << conn_id;
                    sprintf(filename, "%s", ss.str().c_str());
                    connection->fp = fopen(filename, "w+"); //RW - overwrite file
                    if (connection->fp == NULL) {
                        perror("Unable to open file, redirecting to STDOUT");
                        connection->fp = stdout;
                        ping_msg->code = CNT_OUTPUT_REDIR;
                    } else {
                        if (setup->self_check() == SETUP_CHCK_VER) fprintf(connection->fp, "%s", setup->get_version().c_str());
                    }
                } else {
                    connection->fp = setup->getFP();
                }
                ping_msg->code = CNT_FNAME_OK;
            }
            if (ping_msg->code == CNT_DONE) {
                ping_msg->code = CNT_DONE_OK;
                ping_msg->count = connection->pkt_cnt;
                setup->setExtFilename(string());
            }
            if (ping_msg->code == CNT_FNAME_OK) {
                message.str("");
                message << endl << ".::. Test from " << client_ip << " started. \t\t[";
                setup->setAntiAsym(false);
                if (setup->extFilenameLen()) {
                    message << "F";
                    connection->F_par = true;
                }
                if (ping_msg->params & CNT_HPAR) {
                    setup->setHPAR(true);
                    connection->H_par = true;
                    message << "H";

                } else {
                    setup->setHPAR(false);
                }
                if (ping_msg->params & CNT_DPAR) {
                    connection->D_par = true;
                    message << "D";
                } else {
                    connection->D_par = setup->showTimeStamps();
                }
                if (ping_msg->params & CNT_ePAR) {
                    connection->e_par = true;
                    message << "e";
                } else {
                    connection->e_par = setup->showBitrate();
                }

                if (ping_msg->params & CNT_EPAR) {
                    connection->E_par = true;
                    message << "E";
                } else {
                    connection->E_par = setup->showSendBitrate();
                }

                if (ping_msg->params & CNT_WPAR) {
                    setup->setWPAR(true);
                    connection->W_par = true;
                    message << "W";
                } else {
                    setup->setWPAR(false);
                }
                if (ping_msg->params & CNT_XPAR) {
                    setup->setXPAR(false);
                    connection->X_par = false;
                    connection->AX_par = true;
                    message << "X";
                    setup->setPayoadSize(ping_msg->size);
                    setup->setAntiAsym(true);
                } else {
                    setup->restoreXPAR();
                }
                if (ping_msg->params & CNT_CPAR) {
                    setup->setCPAR(true);
                    connection->C_par = true;
                    message << "C";
                } else {
                    setup->setCPAR(false);
                }
                if (ping_msg->params & CNT_JPAR) {
                    setup->setJPAR(true);
                    connection->J_par = true;
                    message << "J";
                } else {
                    setup->setCPAR(false);
                }
                message << "]";
                cerr << message.str() << endl;
                if (show) {
                    ss.str("");
                    if (setup->toCSV()) {
                        sprintf(msg, "S_TimeStamp;S_PacketSize;S_From;S_Sequence;S_TTL;S_Delta;S_RX_Rate;S_TX_Rate;\n");
                    }
                    ss << msg;
                    if (setup->useTimedBuffer()) {
                        connection->msg_store.push_back(ss.str());
                    } else {
                        fprintf(connection->fp, "%s", ss.str().c_str());
                    }
                }
            }

            ret_size = 64;
            sendto(this->sock, packet, ret_size, 0, (struct sockaddr *) &saClient, addr_len);
            connection->refTv = connection->curTv;
            clock_gettime(CLOCK_REALTIME, &connection->curTv);

            //Clean UP
            if (ping_msg->code == CNT_DONE_OK) {
                if (setup->useTimedBuffer(connection->W_par)) {
                    if (setup->extFilenameLen()) {
                        cerr << "     ~ Writeing data into file: " << setup->getExtFilename() << endl;
                    } else {
                        if (setup->outToFile()) {
                            cerr << "     ~ Writeing data into file: " << setup->getFilename() << endl;
                        }
                    }
                    string tmp_str;
                    for (unsigned int i = 0; i < connection->msg_store.size(); i++) {
                        tmp_str = connection->msg_store[i];
                        fprintf(connection->fp, "%s", tmp_str.c_str());
                    }
                    connection->msg_store.clear();
                }
                if (connection->fp != stdout) {
                    fclose(connection->fp);
                }
                cerr << ".::. Test from " << client_ip << " finished.  ~  " << connection->pkt_cnt << " packets processed." << endl;
                connection->fp = stdout;
                delete connection;
                connections.erase(conn_id);
#ifndef _NOSTATS
                if (stats->connStatRemove(conn_id)) {
                    std::cerr << "cServerStats::connStatRemove FAILED - conn_id [" << conn_id << "]" << endl;
                }
#endif
            }
        }
    }
    return 1;
}

void cServer::terminate() {
    this->stop = true;
}
