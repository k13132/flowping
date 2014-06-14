/* 
 * File:   client.cpp
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

#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <sstream>
#include <vector>
#include <string>
#include <string.h>

#include "cClient.h"

cClient::cClient(cSetup *setup) {
    this->setup = setup;
    this->s_running = false;
    this->r_running = false;
    if (pthread_barrier_init(&barr, NULL, 2)) //2 == Sender + Receiver
    {
        printf("Could not create a barrier\n");
    }
    this->started = false;
    this->stop = false;
    this->done = false;
    this->rtt_avg = -1;
    this->rtt_min = -1;
    this->rtt_max = -1;
    this->pkt_rcvd = 0;
    this->pkt_sent = 0;
    this->last_seq_rcv = 0;
    this->ooo_cnt = 0;
    this->t1 = (u_int64_t) (setup->getTime_t()*1000000.0); //convert s to us
    this->t2 = (u_int64_t) (this-> t1 + setup->getTime_T()*1000000.0); //convert s to us
    this->t3 = (u_int64_t) (setup->getTime_R()*1000000.0); //convert s to us
    this->base_interval = (u_int64_t) setup->getInterval_i();
    this->min_interval = setup->getMinInterval();
    this->max_interval = setup->getMaxInterval();
    this->bchange = setup->getBchange();
    //cerr << "T1="<<this->t1<<"\tT2="<<this->t2<<"\tT3="<<this->t3<<endl;
}

cClient::~cClient() {
    close(this->sock);
}

int cClient::run_receiver_output() {
    while (!done) {
        usleep(200);
    }
    return 0;
}

int cClient::run_receiver() {
    event_t event;
    struct hostent *hp;
    stringstream ss;
    this->r_running = true;
    bool show = not setup->silent();
    cout << ".::. Pinging " << setup->getHostname() << " with " << setup->getPacketSize() << " bytes of data:" << endl;

    // Find the server
    // Convert the host name as a dotted-decimal number.

    bzero((void *) &saServer, sizeof (saServer));
#ifdef DEBUG
    if (setup->debug()) printf("-D- Looking up %s...\n", setup->getHostname().c_str());
#endif

    if ((hp = gethostbyname(setup->getHostname().c_str())) == NULL) {
        perror("host name error");
        exit(1);
    }
    bcopy(hp->h_addr, (char *) &saServer.sin_addr, hp->h_length);
    //
    // Create a UDP/IP datagram socket
    //

    this->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (this->sock < 0) {
        perror("Failed in creating socket");
        exit(1);
    }
    if (setup->useInterface()) {
        setsockopt(this->sock, SOL_SOCKET, SO_BINDTODEVICE, setup->getInterface().c_str(), strlen(setup->getInterface().c_str()));
        cerr << "Using interface:" << setup->getInterface().c_str() << endl;
    }

    // Fill in the address structure for the server
    saServer.sin_family = AF_INET;
    saServer.sin_port = htons(setup->getPort()); // Port number from command line
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(-1);
    }
    struct ping_pkt_t *ping_pkt;
    struct ping_msg_t *ping_msg;
    unsigned char packet[MAX_PKT_SIZE + 60];
    int nRet;
    ping_pkt = (struct ping_pkt_t*) (packet);
    ping_msg = (struct ping_msg_t*) (packet);

    /*
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask); //To run on CPU 0
    int result = sched_setaffinity(0, sizeof (mask), &mask);
     */


    // Wait for the first reply
    int nFromLen;
    float rtt;
    struct timeval curTv;
    char msg[1000] = "";
    gettimeofday(&curTv, NULL);
    while (!done) {
        //if (stop) break;
        ss.str("");
        nFromLen = sizeof (struct sockaddr);
        nRet = recvfrom(this->sock, packet, MAX_PKT_SIZE, 0, (struct sockaddr *) &saServer, (socklen_t *) & nFromLen);
        if (nRet < 0) {
            perror("receiving");
            close(this->sock);
            exit(1);
        }
        //        if (ping_pkt->type == PING){
        //            rcv_queue.push(ping_pkt&);
        //        }
        if (ping_pkt->type == CONTROL) {
#ifdef DEBUG
            if (setup->debug()) printf("-D- Control packet received! code:%d\n", ping_msg->code);
#endif
            if (ping_msg->code == CNT_DONE_OK) {
                done = true;
                server_received = ping_msg->count;
            }
            if (ping_msg->code == CNT_FNAME_OK) started = true;
            if (ping_msg->code == CNT_OUTPUT_REDIR) started = true;

        }
        if (ping_pkt->type == PING) {
            this->pkt_rcvd++;
            //get current tv
            rrefTv = curTv;
            gettimeofday(&curTv, NULL);
            double delta = ((double) (curTv.tv_sec - rrefTv.tv_sec)*1000.0 + (double) (curTv.tv_usec - rrefTv.tv_usec) / 1000.0);
            //get rtt in millisecond
            if (pkt_rcvd == 1) delta = (this->getInterval() / 1000.0); //First delta shown represents Interval instead of zero value;
            rtt = ((curTv.tv_sec - ping_pkt->sec) * 1000 + (curTv.tv_usec - ping_pkt->usec) / 1000.0);
            if (rtt < 0) perror("wrong RTT value !!!\n");
            //cout << curTv.tv_sec << "\t" << ping_pkt->sec << "\t" << curTv.tv_usec << "\t" << ping_pkt->usec << "\t" << rtt << endl;
            //get tSent in millisecond
            sent_ts = ((ping_pkt->sec - start_ts.tv_sec) * 1000 + (ping_pkt->usec - start_ts.tv_usec) / 1000.0);
            if (rtt_min == -1) {
                rtt_min = rtt;
                rtt_max = rtt;
                rtt_avg = rtt;
            } else {
                if (rtt < rtt_min) {
                    rtt_min = rtt;
                }
                if (rtt > rtt_max) {
                    rtt_max = rtt;
                }
            }
            rtt_avg = ((this->pkt_rcvd - 1) * rtt_avg + rtt) / this->pkt_rcvd;

            if (show) {
                if (setup->wholeFrame()) nRet += 42;
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
                    sprintf(msg, "rx;%d;%s;%d;", nRet, setup->getHostname().c_str(), ping_pkt->seq);
                } else {
                    if (setup->compat()) {
                        sprintf(msg, "%d bytes from %s: icmp_req=%d ttl=0", nRet, setup->getHostname().c_str(), ping_pkt->seq);
                    } else {
                        sprintf(msg, "%d bytes from %s: req=%d", nRet, setup->getHostname().c_str(), ping_pkt->seq);
                    }
                }
                ss << msg;
                if (setup->toCSV()) {
                    sprintf(msg, "%.2f;", rtt);
                } else {
                    sprintf(msg, " time=%.2f ms", rtt);
                }
                ss << msg;
                if (setup->showBitrate()) {
                    if (setup->toCSV()) {
                        sprintf(msg, "%.3f;%.2f;", delta, (1000 / delta) * nRet * 8 / 1000);
                    } else {
                        sprintf(msg, " delta=%.3f ms rx_rate=%.2f kbit/s ", delta, (1000 / delta) * nRet * 8 / 1000);
                    }
                    ss << msg;
                } else {
                    if (setup->toCSV()) {
                        sprintf(msg, ";;");
                        ss << msg;
                    }
                }

            }
            if (last_seq_rcv > ping_pkt->seq) {
                if (!setup->toCSV()) {
                    sprintf(msg, " OUT OF ORDER!\n");
                    ss << msg;
                }
                this->ooo_cnt++;
            } else {
                if (show) {
                    sprintf(msg, "\n");
                    ss << msg;
                }
                if (setup->useTimedBuffer()) {
                    event.ts.sec = curTv.tv_sec;
                    event.ts.usec = curTv.tv_usec;
                    event.msg = ss.str();
                    msg_store.push_back(event);
                } else {
                    fprintf(fp, "%s", ss.str().c_str());
                }
                last_seq_rcv = ping_pkt->seq;
            }

            //Deadline check
            if ((setup->getDeadline() != 0) && (curTv.tv_sec + (curTv.tv_usec / 1000000.0) >= (start_ts.tv_sec + (start_ts.tv_usec / 1000000.0) + setup->getDeadline()))) {
                stop = true;
            }

        }
        //usleep(1);
    }
    //gettimeofday(&curTv, NULL);
    this->tSent = (double) ((curTv.tv_sec - this->start_ts.tv_sec)*1000.0 + (curTv.tv_usec - this->start_ts.tv_usec) / 1000.0);
    if (setup->useTimedBuffer()) {
        cerr << ".::. Writeing data into file." << endl;
        string tmp_str;
        int idx, idx_snd = 0;
        long vmsg_len, vmsg_snd_len;
        vmsg_len = msg_store.size();
        vmsg_snd_len = msg_store_snd.size();
        event_t ev, ev_snd;
        cerr << "     ~ Messages to be writen - mgs_store:" << vmsg_len << endl;
        cerr << "     ~ Messages to be writen - mgs_store_snd:" << vmsg_snd_len << endl;
        while ((idx < vmsg_len) || (idx_snd < vmsg_snd_len)) {
            if ((idx < vmsg_len) && (idx_snd < vmsg_snd_len)) {
                ev = msg_store[idx];
                ev_snd = msg_store_snd[idx_snd];
                if ((ev.ts.sec * 1000000 + ev.ts.usec) <= (ev_snd.ts.sec * 1000000 + ev_snd.ts.usec)) {
                    fprintf(fp, "%s", ev.msg.c_str());
                    idx++;
                } else {
                    fprintf(fp, "%s", ev_snd.msg.c_str());
                    idx_snd++;
                }
            } else {
                if (idx < vmsg_len) {
                    ev = msg_store[idx];
                    fprintf(fp, "%s", ev.msg.c_str());
                    //cerr << ev.ts.sec << "." <<ev.ts.usec << "\t" << ev.msg<<endl;
                    idx++;
                }
                if (idx_snd < vmsg_snd_len) {
                    ev_snd = msg_store_snd[idx_snd];
                    fprintf(fp, "%s", ev_snd.msg.c_str());
                    //cerr << ev_snd.ts.sec << "." <<ev_snd.ts.usec << "\t" << ev_snd.msg<<endl;
                    idx_snd++;
                }
            }
        }
        cerr << "     ~ Messages writen - msg_store:" << idx << endl;
        cerr << "     ~ Messages writen - msg_store_snd:" << idx_snd << endl;
        msg_store.clear();
    }
    if (!setup->toCSV()) this->report();
    this->r_running = false;
    return 1;
}

int cClient::run_sender() {
    this->s_running = true;
    event_t event;
    char msg[1000] = {0};
    struct ping_pkt_t *ping_pkt;
    struct ping_msg_t *ping_msg;
    stringstream ss;
    unsigned char packet[MAX_PKT_SIZE + 60];
    int nRet;
    int interval, cinterval;
    int pipe_handle;
    bool show = not setup->silent();
    /*
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask); //To run on CPU 0
    int result = sched_setaffinity(0, sizeof (mask), &mask);
     */
    if (setup->npipe()) {
        pipe_handle = open("/tmp/flowping", O_RDONLY | O_NONBLOCK);
        if (pipe_handle == -1) {
            fprintf(stdout, "Failed to open the named pipe\n");
            exit(-2);
        }
    }
    bool pipe_started = false;
    char pipe_buffer[MAX_PKT_SIZE];
    memset(pipe_buffer, 0, MAX_PKT_SIZE);

    if (setup->getFilename().length() && setup->outToFile()) {
        fp = fopen(setup->getFilename().c_str(), "w+"); //RW - overwrite file
        if (fp == NULL) {
            perror("Unable to open file, redirecting to STDOUT");
            fp = stdout;
        } else {
            if (setup->self_check() == SETUP_CHCK_VER) fprintf(fp, setup->get_version().c_str());
        }
    } else {
        fp = setup->getFP();
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
    if (setup->getF_Filename().length() && setup->sendFilename()) {
        strcpy(ping_msg->msg, setup->getF_Filename().c_str());
        ping_msg->code = CNT_FNAME;
    } else {
        ping_msg->code = CNT_NOFNAME;
    }
    unsigned int pk_len = HEADER_LENGTH + strlen(ping_msg->msg);
    int timeout = 0;
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(-1);
    }
    while (!started) {
#ifdef DEBUG        
        if (setup->debug()) {
            cout << "Sending CONTROL Packet - code:" << ping_msg->code << endl;
        }
#endif        
        nRet = sendto(this->sock, packet, pk_len, 0, (struct sockaddr *) &saServer, sizeof (struct sockaddr));
        if (nRet < 0) {
            perror("sending");
            close(this->sock);
            exit(1);
        }
        usleep(200000); //  5pkt/s
        timeout++;
        if (timeout == 50) { //try contact server fo 10s
            printf("Can't connect to server\n");
            exit(1);
        }
    }

    ping_pkt->type = PING; //prepare the first packet
    gettimeofday(&this->start_ts, NULL);
    gettimeofday(&this->my_ts, NULL);
    my_ts.tv_sec += setup->getTime_t();
    ping_pkt->sec = start_ts.tv_sec;
    ping_pkt->usec = start_ts.tv_usec;

    u_int16_t payload_size;
    bool speedup = setup->speedUP();
#ifdef DEBUG
    if (speedup and setup->debug()) {
        cout << "-D- Speed_UP active" << endl;
    }
#endif    
    int correction = 0;
    int pipe_cnt = 0;
    //struct timeval TA, TB;
    struct timespec tt;
    gettimeofday(&refTv, NULL);
    clock_gettime(CLOCK_MONOTONIC, &tt);
    u_int64_t speed_int = this->getInterval();
    cinterval = speed_int * 1000L;
    //cout << "A" << speed_int << endl;
    if (cinterval + tt.tv_nsec > 999999999) {
        req.tv_sec = tt.tv_sec + 1;
        req.tv_nsec = ((cinterval + tt.tv_nsec) - 1000000000);
    } else {
        req.tv_sec = tt.tv_sec;
        req.tv_nsec = ((cinterval + tt.tv_nsec));
    }
    //S_PacketSize;S_From;S_Sequence;S_TTL;S_Delta;S_RX_Rate;S_TX_Rate; 
    if (show) {
        if (setup->toCSV()) {
            ss.str("");
            sprintf(msg, "C_TimeStamp;C_Direction;C_PacketSize;C_From;C_Sequence;C_RTT;C_Delta;C_RX_Rate;C_To;C_TX_Rate;\n");
            ss << msg;
            if (setup->useTimedBuffer()) {
                event.ts.sec = curTv.tv_sec;
                event.ts.usec = curTv.tv_usec;
                event.msg = ss.str();
                msg_store_snd.push_back(event);
            } else {
                fprintf(fp, "%s", ss.str().c_str());
            }
        }
    }

    timeval ts;
    timed_packet_t tinfo;

    u_int64_t curTime = 0;
    u_int64_t tgTime = 0;

    for (int i = 1; i <= setup->getCount(); i++) {
        ping_pkt->seq = i;
        //deadline reached [ -w ];
        if (stop) break;

        if (setup->useTimedBuffer()) {
            if (!setup->nextPacket()) {
                tinfo = setup->getNextPacket();
                payload_size = tinfo.len - HEADER_LENGTH;
            } else {
                stop = true;
                break;
            }
        } else {
            payload_size = this->getPacketSize() - HEADER_LENGTH;
        }
        pkt_sent++;
        if (setup->npipe()) {
            payload_size = read(pipe_handle, pipe_buffer, payload_size);
            if (!pipe_started) {
                if (payload_size == 0) {
                    usleep(200000);
                    pipe_cnt++;
                    if (pipe_cnt++ < 3000) continue; //10 minutes timeout.
                } else {
                    pipe_started = true;
                    i = 1;
                    pkt_sent = 1;
                }
            }
            if (payload_size > 0) {
                memcpy(ping_pkt->padding, pipe_buffer, payload_size);
            }
            if (setup->frameSize()) {
                if (payload_size < (this->getPacketSize() - (HEADER_LENGTH + 42))) {
                    stop = true;
                }
            } else {
                if (payload_size < (this->getPacketSize() - HEADER_LENGTH)) {
                    stop = true;
                }

            }
        }
        gettimeofday(&ts, NULL);
        ping_pkt->sec = ts.tv_sec;
        ping_pkt->usec = ts.tv_usec;
        if (setup->isAntiAsym()) {
            payload_size = 0;
        }

        if (setup->frameSize()){
            payload_size-=42;
        }
        nRet = sendto(this->sock, packet, HEADER_LENGTH + payload_size, 0, (struct sockaddr *) &saServer, sizeof (struct sockaddr));
        if (nRet < 0) {
            cerr << "Packet size:" << HEADER_LENGTH + payload_size << endl;
            perror("sending");
            close(this->sock);
            exit(1);
        }
        if (setup->useTimedBuffer()) {
            tgTime = (uint64_t) ((uint64_t) start_ts.tv_usec + ((uint64_t) start_ts.tv_sec) * 1000000)+(tinfo.usec + tinfo.sec * 1000000);
            //cout << sizeof(start_ts.tv_sec)<<endl;
            gettimeofday(&curTv, NULL);
            while (((uint64_t) curTv.tv_usec + ((uint64_t) curTv.tv_sec) * 1000000) < tgTime) {
                gettimeofday(&curTv, NULL);
            }
            delta = (((double) (curTv.tv_sec - refTv.tv_sec)*1000000 + (curTv.tv_usec - refTv.tv_usec)));
            gettimeofday(&refTv, NULL);
        } else {
            if (speedup) {
                interval = speed_int;
            } else {
                interval = this->getInterval();
            }
            if (setup->actWaiting()) {
                gettimeofday(&curTv, NULL);
                curTime = (uint64_t) curTv.tv_usec + ((uint64_t) curTv.tv_sec) * 1000000;
                tgTime = curTime + interval + correction;
                while (curTime < tgTime) {
                    gettimeofday(&curTv, NULL);
                    curTime = (uint64_t) curTv.tv_usec + ((uint64_t) curTv.tv_sec) * 1000000;
                }
                gettimeofday(&curTv, NULL);
                //delta = ((double) (curTv.tv_sec - refTv.tv_sec)*1000.0 + (double) (curTv.tv_usec - refTv.tv_usec) / 1000.0);

                //Delta in nsec
                delta = ((double) (curTv.tv_sec - refTv.tv_sec)*1000000 + (curTv.tv_usec - refTv.tv_usec));
                gettimeofday(&refTv, NULL);
                correction = (int) ((interval + correction) - delta);
                if (-correction > interval - 1) {
                    correction = -(interval - 1);
                }
            } else {
#ifdef DEBUG
                if (setup->debug()) cout << interval << "\t" << correction << "\t" << delta << endl;
                gettimeofday(&refTv, NULL);
#endif

                //delay(interval);
#ifdef DEBUG
                gettimeofday(&curTv, NULL);
                delta = ((double) (curTv.tv_sec - refTv.tv_sec)*1000.0 + (double) (curTv.tv_usec - refTv.tv_usec) / 1000.0);
#endif

                delay(req);
                clock_gettime(CLOCK_MONOTONIC, &tt);
                gettimeofday(&curTv, NULL);
                delta = ((double) (curTv.tv_sec - refTv.tv_sec)*1000000 + (curTv.tv_usec - refTv.tv_usec));
                //delta = ((double) (curTv.tv_sec - refTv.tv_sec)*1000.0 + (double) (curTv.tv_usec - refTv.tv_usec) / 1000.0);
                gettimeofday(&refTv, NULL);
                if (pkt_sent == 1) delta = interval; //First delta shown represents Interval instead of zero value;            
                correction = (int) ((interval + correction) - delta);
                if (-correction > interval - 1) {
                    correction = -(interval - 1);
                }
                cinterval = (interval + correction) * 1000L;
                req.tv_sec = tt.tv_sec + (cinterval + tt.tv_nsec) / 1000000000L;
                req.tv_nsec = (cinterval + tt.tv_nsec) % 1000000000L;
            }
        }
        if (setup->showSendBitrate()) {
            if (setup->frameSize()) nRet += 42;
            ss.str("");
            memset(msg, 0, sizeof (msg));
            //"C_TimeStamp;RX/TX;C_PacketSize;C_From;C_Sequence;C_RTT;C_Delta;C_RX_Rate;C_To;C_TX_Rate;"
            if (setup->showTimeStamps()) {
                if (setup->toCSV()) {
                    sprintf(msg, "%d.%06d;", ts.tv_sec, ts.tv_usec);
                } else {
                    sprintf(msg, "[%d.%06d] ", ts.tv_sec, ts.tv_usec);
                }
            } else {
                if (setup->toCSV()) {
                    sprintf(msg, ";");
                }
            }
            ss << msg;
            if (setup->toCSV()) {
                sprintf(msg, "tx;%d;;%d;;", nRet, ping_pkt->seq);
                ss << msg;
                sprintf(msg, "%.3f;;%s;%.2f;;;\n", (delta / 1000.0), setup->getHostname().c_str(), (1000.0 / delta) * (nRet) * 8);
                ss << msg;
            } else {
                sprintf(msg, "%d bytes to %s: req=%d ", nRet, setup->getHostname().c_str(), ping_pkt->seq);
                ss << msg;
                sprintf(msg, "delta=%.3f ms tx_rate=%.2f kbit/s \n", delta / 1000.0, (1000.0 / delta) * (nRet) * 8);
                ss << msg;
            }
            if (setup->useTimedBuffer()) {
                event.ts.sec = curTv.tv_sec;
                event.ts.usec = curTv.tv_usec;
                event.msg = ss.str();
                msg_store_snd.push_back(event);
            } else {
                fprintf(fp, "%s", ss.str().c_str());
            }
        }
    }
    ping_pkt->type = CONTROL;
    ping_msg->code = CNT_DONE;
    timeout = 0;
    //sleep(2); //wait for network congestion "partialy" disapear.
    while (!done) {
        nRet = sendto(this->sock, packet, pk_len, 0, (struct sockaddr *) &saServer, sizeof (struct sockaddr));
        if (nRet < 0) {
            perror("sending");
            close(this->sock);
            exit(1);
        }
        usleep(200000); //  5pkt/s
        timeout++;
        if (timeout == 150) { //30s
            printf("Can't get stats from server\n");
            exit(1);
        }
    }
    close(pipe_handle);
#ifdef DEBUG
    cout << setup->getInterval() << endl;
#endif
    this->s_running = false;
    return 0;
}

void cClient::report() {
    float p = (100.0 * (pkt_sent - pkt_rcvd)) / pkt_sent;
    float uloss = (float) (100.0 * (pkt_sent - server_received)) / pkt_sent;
    float dloss = (float) (100.0 * (server_received - pkt_rcvd)) / server_received;

    fprintf(fp, "\n---Client report--- %s ping statistics ---\n", setup->getHostname().c_str());
    fprintf(fp, "%llu packets transmitted, %llu received, %.f%% packet loss, time %.fms\n", pkt_sent, pkt_rcvd, p, tSent);
    fprintf(fp, "rtt min/avg/max = %.2f/%.2f/%.2f ms, ", rtt_min, rtt_avg, rtt_max);
    fprintf(fp, "Out of Order = %llu packets\n", ooo_cnt);

    fprintf(fp, "\n---Server report--- %s ping statistics ---\n", setup->getHostname().c_str());
    fprintf(fp, "%llu received, %.f%% upstream packet loss, %.f%% downstream packet loss\n", server_received, uloss, dloss);
    fclose(fp);
}

void cClient::terminate() {
    this->stop = true;
}

u_int64_t cClient::getInterval(void) {
    struct timeval cur_ts;
    u_int64_t interval;
    if (setup->pkSizeChange()) {
        interval = this->base_interval;
    } else {
        gettimeofday(&cur_ts, NULL);
        if (setup->shape()) {
            time = ((cur_ts.tv_sec * 1000000 + cur_ts.tv_usec)-(this->start_ts.tv_sec * 1000000 + this->start_ts.tv_usec));
            // ! setup->getRTBitrate(time) set PayloadSize for setup->getPacketSize()
            double tmp2 = setup->getRTBitrate(time); // Have to be First
            double tmp1 = (double) setup->getPacketSize()*(double) 8000000; // Have to be Second
            interval = (u_int64_t) (tmp1 / tmp2);
            //cout << "Stage 1 - inteval:" << interval << "\ttime:" << time << "\ttmp1:" << tmp1 << "\ttmp2:" << tmp2 << endl;
        } else {
            time = ((cur_ts.tv_sec * 1000000 + cur_ts.tv_usec)-(this->start_ts.tv_sec * 1000000 + this->start_ts.tv_usec)) % this->t2;
            if (time<this->t1) {
                interval = this->base_interval;
            } else {
                double tmp1 = (double) setup->getPacketSize()*(double) 8000000;
                double tmp2 = (double) setup->getBaseRate()+(double) (time - this->t1) * (double) this->bchange;
                interval = (u_int64_t) (tmp1 / tmp2);
                //cerr << "\e[0;31m DEBUG \e[1;37m INTERVAL = " << interval << " TMP1 = " << tmp1 << " TMP2 = " << tmp2 << " TIME = " << time << " BRATE  = " << setup->getBaseRate() << " BCHANGE = " << this->bchange << "\e[0m" << endl;
            }

#ifdef DEBUG
            if (setup->debug()) cout << "-D- * T2 * ";
            //cout << "Bitrate:" << setup->getBaseRate()+(time - this->t1) * this->bchange << endl;
#endif
            //Check interval range - .
            if (interval<this->min_interval) {
#ifdef DEBUG
                if (setup->debug()) {
                    cout << "-D- MinInterval Correction: " << interval << endl;
                }
#endif
                interval = this->min_interval;
            }
            if (interval>this->max_interval) {
#ifdef DEBUG
                if (setup->debug()) {
                    cout << "-D- MaxInterval Correction: " << interval << endl;
                }
#endif
                interval = this->max_interval;
            }
        }



    }
#ifdef DEBUG    
    if (setup->debug()) {
        cout << "-D- Interval: " << interval << endl;
        cout << "-D- bChange: " << this->bchange << endl;
        cout << "-D- BaseRate: " << setup->getBaseRate() << endl;
    }
#endif    
    return interval;
}

u_int16_t cClient::getPacketSize() {
    struct timeval cur_ts;
    u_int16_t payload_size = setup->getPacketSize();
    if (setup->pkSizeChange()) {
        gettimeofday(&cur_ts, NULL);
        time = ((cur_ts.tv_sec * 1000000 + cur_ts.tv_usec)-(this->start_ts.tv_sec * 1000000 + this->start_ts.tv_usec)) % this->t2;
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
    int e = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &req, &req);
}

bool cClient::status() {
    return (this->r_running || this->s_running);
}