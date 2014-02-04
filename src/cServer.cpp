/* 
 * File:   server.cpp
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



#include <stdlib.h>
#include <sstream>
#include "cServer.h"
#include "_types.h"

cServer::cServer(cSetup *setup) {
    this->setup = setup;
    this->stop = false;
}

cServer::~cServer() {
    close(this->sock);
}

int cServer::run() {
    struct sockaddr_in saServer;
    FILE * fp = setup->getFP();
    u_int64_t count = 0;
    bool show = not setup->silent();
    char msg[200] = "";
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

    // Wait for data
    int addr_len;
    addr_len = sizeof (sockaddr);
    struct ping_pkt_t *ping_pkt;
    struct ping_msg_t *ping_msg;
    unsigned char packet[MAX_PKT_SIZE + 60];
    struct sockaddr_in saClient;
    struct timeval curTv, refTv;
    refTv.tv_sec = 0;
    unsigned int cnt = 0;

    int ip;
    char client_ip[INET_ADDRSTRLEN];
    while (!stop) {
        rec_size = ret_size = recvfrom(this->sock, packet, MAX_PKT_SIZE, 0, (struct sockaddr *) &saClient, (socklen_t *) & addr_len);
#ifdef DEBUG        
        if (setup->debug()) cout << ret_size << " bytes received." << endl;
#endif        
        ping_pkt = (struct ping_pkt_t*) (packet);
        if (ping_pkt->type == PING) {
            if (setup->isAsym()) ret_size = MIN_PKT_SIZE;
            sendto(this->sock, packet, ret_size, 0, (struct sockaddr *) &saClient, addr_len);
            if (show) {
                refTv = curTv;
                gettimeofday(&curTv, NULL);
                double delta = ((double) (curTv.tv_sec - refTv.tv_sec)*1000.0 + (double) (curTv.tv_usec - refTv.tv_usec) / 1000.0);
                ss.str("");
                if (setup->showTimeStamps()) {
                    if (setup->toCSV()) {
                        sprintf(msg, "%d.%06d;", curTv.tv_sec, curTv.tv_usec);
                    } else {
                        sprintf(msg, "[%d.%06d] ", curTv.tv_sec, curTv.tv_usec);
                    }
                    ss << msg;
                } else {
                    if (setup->toCSV()) {
                        sprintf(msg, ";");
                        ss << msg;
                    }
                }
                if (setup->toCSV()) {
                    sprintf(msg, "%d;%s;%d;xx;%.3f;", rec_size, client_ip, ping_pkt->seq, delta);
                } else {
                    sprintf(msg, "%d bytes from %s: req=%d ttl=xx delta=%.3f ms", rec_size, client_ip, ping_pkt->seq, delta);
                }
                ss << msg;
                if (setup->showBitrate()) {
                    if (setup->wholeFrame()) {
                        if (setup->toCSV()) {
                            sprintf(msg, "%.2f;", (1000 / delta) * (rec_size + 42) * 8 / 1000);
                        } else {
                            sprintf(msg, " rx_rate=%.2f kbit/s", (1000 / delta) * (rec_size + 42) * 8 / 1000);
                        }
                        ss << msg;
                        if (setup->toCSV()) {
                            sprintf(msg, "%.2f;", (1000 / delta) * (ret_size + 42) * 8 / 1000);
                        } else {
                            sprintf(msg, " tx_rate=%.2f kbit/s", (1000 / delta) * (ret_size + 42) * 8 / 1000);
                        }
                        ss << msg;
                    } else {
                        if (setup->toCSV()) {
                            sprintf(msg, "%.2f;", (1000 / delta) * rec_size * 8 / 1000);
                        } else {
                            sprintf(msg, " rx_rate=%.2f kbit/s", (1000 / delta) * rec_size * 8 / 1000);
                        }
                        ss << msg;
                        if (setup->toCSV()) {
                            sprintf(msg, "%.2f;", (1000 / delta) * ret_size * 8 / 1000);
                        } else {
                            sprintf(msg, " tx_rate=%.2f kbit/s", (1000 / delta) * ret_size * 8 / 1000);
                        }
                        ss << msg;
                    }
                } else {
                    if (setup->toCSV()) {
                        sprintf(msg, ";;");
                        ss << msg;
                    }
                }
                sprintf(msg, "\n");
                ss << msg;
                if (setup->useTimedBuffer()) {
                    msg_store.push_back(ss.str());
                } else {
                    fprintf(fp, "%s", ss.str().c_str());

                }

            }
            count++;
        }
        if (ping_pkt->type == CONTROL) {
            ip = (saClient.sin_addr.s_addr);
            inet_ntop(AF_INET, &ip, client_ip, INET_ADDRSTRLEN);
            ping_msg = (struct ping_msg_t*) (packet);
#ifdef DEBUG
            if (setup->debug()) cout << "Control packet received! code:" << (int) ping_msg->code << endl;
#endif
            if (ping_msg->code == CNT_FNAME) {
                fp = fopen(ping_msg->msg, "w+"); //RW - overwrite file
                ping_msg->code = CNT_FNAME_OK;
                if (fp == NULL) {
                    perror("Unable to open file, redirecting to STDOUT");
                    fp = stdout;
                    ping_msg->code = CNT_OUTPUT_REDIR;
                } else {
                    setup->setExtFilename((string) ping_msg->msg);
                    if (setup->self_check() == SETUP_CHCK_VER) fprintf(fp, setup->get_version().c_str());
                }
            }
            if (ping_msg->code == CNT_NOFNAME) {
                if (setup->getFilename().length() && setup->outToFile()) {
                    fp = fopen(setup->getFilename().c_str(), "w+"); //RW - overwrite file
                    if (fp == NULL) {
                        perror("Unable to open file, redirecting to STDOUT");
                        fp = stdout;
                        ping_msg->code = CNT_OUTPUT_REDIR;
                    } else {
                        if (setup->self_check() == SETUP_CHCK_VER) fprintf(fp, setup->get_version().c_str());
                    }
                } else {
                    fp = setup->getFP();
                }
                ping_msg->code = CNT_FNAME_OK;
            }
            if (ping_msg->code == CNT_DONE) {
                ping_msg->code = CNT_DONE_OK;
                ping_msg->count = count;
                setup->setExtFilename(string());
            }
            if (ping_msg->code == CNT_FNAME_OK) {
                message.str("");
                message << endl << ".::. Test from " << client_ip << " started. \t\t[";
                if (setup->extFilenameLen()) {
                    message << "F";
                }
                if (ping_msg->params & CNT_HPAR) {
                    setup->setHPAR(true);
                    message << "H";
                } else {
                    setup->setHPAR(false);
                }
                if (ping_msg->params & CNT_WPAR) {
                    setup->setWPAR(true);
                    message << "W";
                } else {
                    setup->setWPAR(false);
                }
                if (ping_msg->params & CNT_CPAR) {
                    setup->setCPAR(true);
                    message << "C";
                } else {
                    setup->setCPAR(false);
                }
                message << "]";
                cout << message.str() << endl;
                if (show) {
                    ss.str("");
                    if (setup->toCSV()) {
                        sprintf(msg, "S_TimeStamp;S_PacketSize;S_From;S_Sequence;S_TTL;S_Delta;S_RX_Rate;S_TX_Rate;\n");
                    }
                    ss << msg;
                    if (setup->useTimedBuffer()) {
                        msg_store.push_back(ss.str());
                    } else {
                        fprintf(fp, "%s", ss.str().c_str());
                    }
                }
            }

            ret_size = 64;
            sendto(this->sock, packet, ret_size, 0, (struct sockaddr *) &saClient, addr_len);
            gettimeofday(&curTv, NULL);
            if (ping_msg->code == CNT_DONE_OK) {
                if (setup->useTimedBuffer()) {
                    if (setup->extFilenameLen()) {
                        cerr << "     ~ Writeing data into file: " << setup->getExtFilename() << endl;
                    } else {
                        if (setup->outToFile()) {
                            cerr << "     ~ Writeing data into file: " << setup->getFilename() << endl;
                        }
                    }
                    string tmp_str;
                    for (int i = 0; i < msg_store.size(); i++) {
                        tmp_str = msg_store[i];
                        fprintf(fp, "%s", tmp_str.c_str());
                    }
                    msg_store.clear();
                }
                if (fp != stdout) {
                    fclose(fp);
                }
                cerr << "     ~ " << count << " packets processed." << endl;
                cerr << ".::. Test from " << client_ip << " finished." << endl;
                fp = stdout;
                refTv.tv_sec = 0;
                count = 0;
            }
        }
    }
    return 1;
}

void cServer::terminate() {
    this->stop = true;
}
