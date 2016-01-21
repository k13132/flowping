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
    this->t1 = (u_int64_t) (setup->getTime_t()*1000000000.0); //convert s to ns
    this->t2 = (u_int64_t) (this-> t1 + setup->getTime_T()*1000000000.0); //convert s to ns
    this->t3 = (u_int64_t) (setup->getTime_R()*1000000000.0); //convert s to ns
    this->base_interval = (u_int64_t) setup->getInterval_i();
    this->min_interval = setup->getMinInterval();
    this->max_interval = setup->getMaxInterval();
    this->bchange = setup->getBchange();
    this->pktBufferReady = false;
}

cClient::~cClient() {
    close(this->sock);
}

int cClient::run_packetFactory() {
    bool pkt_created;
    while (!setup->tpReady()) {
        usleep(200000);
    }
    if (!setup->useTimedBuffer()) {
        while (!done && !stop) {
            pkt_created = setup->prepNextPacket();
            while (pkt_created && ((setup->getTimedBufferDelay() < 5000000000) || (setup->getTimedBufferSize() < 1000))) {
                pkt_created = setup->prepNextPacket();
                if (setup->getTimedBufferSize()>50000){
                    break;
                }
                if (stop){
                    return 0;
                }
            }
            if (!pkt_created) {
                return 0;
            }
            if (setup->getTimedBufferSize()) {
                this->pktBufferReady = true;
            }
            usleep(setup->getTimedBufferDelay()/4000);
            cout << "WORK! "<<setup->getTimedBufferDelay()/1000000000.0<<endl;
        }
    } else {
        while (setup->prepNextPacket());
        cout << "Number of packets in buffer:"<< setup->getTimedBufferSize()<<endl;
        if (setup->getTimedBufferSize()) {
            this->pktBufferReady = true;
        }
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
    printf("-D- Looking up %s...\n", setup->getHostname().c_str());
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
    char msg[1000] = "";
    clock_gettime(CLOCK_REALTIME, &r_curTv);
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
            printf("-D- Control packet received! code:%d\n", ping_msg->code);
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
            r_refTv = r_curTv;
            clock_gettime(CLOCK_REALTIME, &r_curTv);
            double r_delta = ((double) (r_curTv.tv_sec - r_refTv.tv_sec)*1000.0 + (double) (r_curTv.tv_nsec - r_refTv.tv_nsec) / 1000000.0);
            //get rtt in millisecond
            //if (pkt_rcvd == 1) r_delta = (this->getInterval() / 1000000.0); //First delta shown represents Interval instead of zero value;
            rtt = ((r_curTv.tv_sec - ping_pkt->sec) * 1000 + (r_curTv.tv_nsec - ping_pkt->nsec) / 1000000.0);
            if (rtt < 0) perror("wrong RTT value !!!\n");
            //cout << curTv.tv_sec << "\t" << ping_pkt->sec << "\t" << curTv.tv_usec << "\t" << ping_pkt->usec << "\t" << rtt << endl;
            //get tSent in millisecond
            sent_ts = ((ping_pkt->sec - start_ts.tv_sec) * 1000 + (ping_pkt->nsec - start_ts.tv_nsec) / 1000000.0);
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
                        sprintf(msg, "%d.%09d;", r_curTv.tv_sec, r_curTv.tv_nsec);
                    } else {
                        sprintf(msg, "[%d.%09d] ", r_curTv.tv_sec, r_curTv.tv_nsec);
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
                        sprintf(msg, "%.3f;%.2f;", r_delta, (1000 / r_delta) * nRet * 8 / 1000);
                    } else {
                        sprintf(msg, " delta=%.3f ms rx_rate=%.2f kbit/s ", r_delta, (1000 / r_delta) * nRet * 8 / 1000);
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
                    event.ts.sec = r_curTv.tv_sec;
                    event.ts.nsec = r_curTv.tv_nsec;
                    event.msg = ss.str();
                    msg_store.push_back(event);
                } else {
                    fprintf(fp, "%s", ss.str().c_str());
                }
                last_seq_rcv = ping_pkt->seq;
            }

            //Deadline check
            if ((setup->getDeadline() != 0) && (r_curTv.tv_sec + (r_curTv.tv_nsec / 1000000000.0) >= (start_ts.tv_sec + (start_ts.tv_nsec / 1000000000.0) + setup->getDeadline()))) {
                stop = true;
            }

        }
        //usleep(1);
    }
    //gettimeofday(&curTv, NULL);
    this->tSent = (double) ((r_curTv.tv_sec - this->start_ts.tv_sec)*1000.0 + (r_curTv.tv_nsec - this->start_ts.tv_nsec) / 1000000.0);
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
                if ((ev.ts.sec * 1000000000 + ev.ts.nsec) <= (ev_snd.ts.sec * 1000000000 + ev_snd.ts.nsec)) {
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
    u_int64_t interval, cinterval;
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
    if (setup->getFilename().length() > 255) {
        perror("Filename to long, exiting.");
        exit(1);
    }
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
        cout << "Sending CONTROL Packet - code:" << ping_msg->code << endl;
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
    clock_gettime(CLOCK_REALTIME, &start_ts);
    my_ts.tv_sec += setup->getTime_t();
    ping_pkt->sec = start_ts.tv_sec;
    ping_pkt->nsec = start_ts.tv_nsec;
    u_int16_t payload_size;
    bool speedup = setup->speedUP();

    int64_t correction = 0;
    int pipe_cnt = 0;
    clock_gettime(CLOCK_REALTIME, &refTv);

    u_int64_t speed_int;
    if (speedup) {
        //speed_int = this->getInterval();
    }
    if (show) {
        if (setup->toCSV()) {
            ss.str("");
            sprintf(msg, "C_TimeStamp;C_Direction;C_PacketSize;C_From;C_Sequence;C_RTT;C_Delta;C_RX_Rate;C_To;C_TX_Rate;\n");
            ss << msg;
            if (setup->useTimedBuffer()) {
                event.ts.sec = refTv.tv_sec;
                event.ts.nsec = refTv.tv_nsec;
                event.msg = ss.str();
                msg_store_snd.push_back(event);
            } else {
                fprintf(fp, "%s", ss.str().c_str());
            }
        }
    }

    timespec ts;
    timed_packet_t tinfo;

    u_int64_t curTime = 0;
    u_int64_t tgTime = 0;

    delta = 0;
    while (!pktBufferReady) {
        usleep(200000);
    }
    for (int i = 1; i <= setup->getCount(); i++) {
        //deadline reached [ -w ];
        if (stop) break;

        refTv = sentTv;
        clock_gettime(CLOCK_REALTIME, &sentTv);
        curTv = sentTv;
        ping_pkt->seq = i;
        delta = (((double) (sentTv.tv_sec - refTv.tv_sec)*1000000000L + (sentTv.tv_nsec - refTv.tv_nsec)));

        //todo optimize +*
        //cout << "---> Buffer state: " << setup->getTimedBufferSize() << endl;
        if (!setup->nextPacket()) {
            tinfo = setup->getNextPacket();
            payload_size = tinfo.len - HEADER_LENGTH;
            tgTime = (uint64_t) ((uint64_t) start_ts.tv_nsec + ((uint64_t) start_ts.tv_sec) * 1000000000)+(tinfo.nsec + tinfo.sec * 1000000000);
            //if (setup->useTimedBuffer()) {

                while (((uint64_t) curTv.tv_nsec + ((uint64_t) curTv.tv_sec) * 1000000000L) < tgTime) {
                    clock_gettime(CLOCK_REALTIME, &curTv);
                }
            //}else{
                //req --> delay
            //}
        } else {
            cout << "Packet buffer is empty."<<endl;
            stop = true;
            break;
        }

        ping_pkt->size = payload_size;
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
        if (setup->isAntiAsym()) {
            payload_size = 0;
        }

        if (setup->frameSize()) {
            payload_size -= 42; //todo check negative size of payload.
        }
        if (this->getPacketSize()) {
            pkt_sent++;
            clock_gettime(CLOCK_REALTIME, &ts);
            ping_pkt->sec = ts.tv_sec;
            ping_pkt->nsec = ts.tv_nsec;
            if (setup->showSendBitrate()) {
                //nPipe??????
                nRet = HEADER_LENGTH + payload_size;
                if (setup->frameSize()) nRet += 42;
                ss.str("");
                memset(msg, 0, sizeof (msg));
                //"C_TimeStamp;RX/TX;C_PacketSize;C_From;C_Sequence;C_RTT;C_Delta;C_RX_Rate;C_To;C_TX_Rate;"
                if (setup->showTimeStamps()) {
                    if (setup->toCSV()) {
                        sprintf(msg, "%d.%09d;", ts.tv_sec, ts.tv_nsec);
                    } else {
                        sprintf(msg, "[%d.%09d] ", ts.tv_sec, ts.tv_nsec);
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
                    sprintf(msg, "%.3f;;%s;%.2f;;;\n", (delta / 1000000.0), setup->getHostname().c_str(), (1000000.0 / delta) * (nRet) * 8);
                    ss << msg;
                } else {
                    sprintf(msg, "%d bytes to %s: req=%d ", nRet, setup->getHostname().c_str(), ping_pkt->seq);
                    ss << msg;
                    sprintf(msg, "delta=%.3f ms tx_rate=%.2f kbit/s \n", delta / 1000000.0, (1000000.0 / delta) * (nRet) * 8);
                    ss << msg;
                }
                if (setup->useTimedBuffer()) {
                    event.ts.sec = curTv.tv_sec;
                    event.ts.nsec = curTv.tv_nsec;
                    event.msg = ss.str();
                    msg_store_snd.push_back(event);
                } else {
                    fprintf(fp, "%s", ss.str().c_str());
                }
            }
            nRet = sendto(this->sock, packet, HEADER_LENGTH + payload_size, 0, (struct sockaddr *) &saServer, sizeof (struct sockaddr));
            if (nRet < 0) {
                cerr << "Packet size:" << HEADER_LENGTH + payload_size << endl;
                perror("sending");
                close(this->sock);
                exit(1);
            }
        } else {
            cout << "PS==0" << endl;
            i--;
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
    int e = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &req, &req);
}

bool cClient::status() {
    return (this->r_running || this->s_running);
}