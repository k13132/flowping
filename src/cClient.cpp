/* 
 * File:   client.cpp
 * 
 * Author: Ondrej Vondrous
 * Email: vondrous@0xFF.cz
 * 
 * Created on 26. červen 2012, 22:37
 */

#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>

#include "cClient.h"

cClient::cClient(cSetup *setup) {
    this->setup = setup;
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
}

cClient::~cClient() {
    close(this->sock);
}

int cClient::run_receiver() {
    struct hostent *hp;
    stringstream ss;
    bool show = not setup->silent();
    cout << "Pinging " << setup->getHostname() << " with " << setup->getPayloadSize() << " bytes of data:" << endl;

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

    // Fill in the address structure for the server
    saServer.sin_family = AF_INET;
    saServer.sin_port = htons(setup->getPort()); // Port number from command line
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(-1);
    }
    struct Ping_Pkt *ping_pkt;
    struct Ping_Msg *ping_msg;
    unsigned char packet[MAX_PKT_SIZE + 60];
    int nRet;
    ping_pkt = (struct Ping_Pkt*) (packet);
    ping_msg = (struct Ping_Msg*) (packet);

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

    while (!done) {
        ss.str("");
        nFromLen = sizeof (struct sockaddr);
        nRet = recvfrom(this->sock, packet, MAX_PKT_SIZE, 0, (struct sockaddr *) &saServer, (socklen_t *) & nFromLen);
        if (nRet < 0) {
            perror("receiving");
            close(this->sock);
            exit(1);
        }

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

            if (rrefTv.tv_sec == 0) {
                gettimeofday(&rrefTv, NULL);
            } else {
                rrefTv = curTv;
            }
            gettimeofday(&curTv, NULL);
            double delta = ((double) (curTv.tv_sec - rrefTv.tv_sec)*1000.0 + (double) (curTv.tv_usec - rrefTv.tv_usec) / 1000.0);
            //get rtt in millisecond

            rtt = ((curTv.tv_sec - ping_pkt->sec) * 1000 + (curTv.tv_usec - ping_pkt->usec) / 1000.0);
            if (rtt < 0) perror("wrong RTT value !!!\n");
            //cout << curTv.tv_sec << "\t" << ping_pkt->sec << "\t" << curTv.tv_usec <<"\t" << ping_pkt->usec <<endl;
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

            struct timeval ts;
            gettimeofday(&ts, NULL);
            if (show) {
                if (setup->showTimeStamps()) {
                    sprintf(msg, "[%d.%06d] ", ts.tv_sec, ts.tv_usec);
                    ss << msg;
                }
                if (setup->compat()) {
                    sprintf(msg, "%d bytes from %s: icmp_req=%d ttl=0 time=%.2f ms", nRet, setup->getHostname().c_str(), ping_pkt->seq, rtt);
                } else {
                    sprintf(msg, "%d bytes from %s: req=%d time=%.2f ms", nRet, setup->getHostname().c_str(), ping_pkt->seq, rtt);
                }
                ss << msg;
                if (setup->showBitrate()) {
                    sprintf(msg, " delta=%.2f ms rx_rate=%.2f kbit/s ", delta, (1000 / delta) * nRet * 8 / 1000);
                    ss << msg;
                }

            }
            if (last_seq_rcv > ping_pkt->seq) {
                sprintf(msg, " OUT OF ORDER!\n");
                ss << msg;
                this->ooo_cnt++;
            } else {
                if (show) {
                    sprintf(msg, "\n");
                    ss << msg;
                }
                fprintf(fp, "%s", ss.str().c_str());

                last_seq_rcv = ping_pkt->seq;
            }

            //Deadline check
            if ((setup->getDeadline() != 0) && (ts.tv_sec + (ts.tv_usec / 1000000.0) >= (start_ts.tv_sec + (start_ts.tv_usec / 1000000.0) + setup->getDeadline()))) {
                stop = true;
            }

        }
        usleep(1);
    }
    gettimeofday(&curTv, NULL);
    this->tSent = (double) ((curTv.tv_sec - this->start_ts.tv_sec)*1000.0 + (curTv.tv_usec - this->start_ts.tv_usec) / 1000.0);
    this->report();
    return 1;
}

int cClient::run_sender() {
    struct Ping_Pkt *ping_pkt;
    struct Ping_Msg *ping_msg;
    stringstream ss;
    char msg[1000] = "";
    unsigned char packet[MAX_PKT_SIZE + 60];
    int nRet;
    int interval, cinterval;
    int pipe_handle;
    /*
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask); //To run on CPU 0
    int result = sched_setaffinity(0, sizeof (mask), &mask);
     */
    if (setup->npipe()) {
        pipe_handle = open("/tmp/uping", O_RDONLY | O_NONBLOCK);
        if (pipe_handle == -1) {
            fprintf(stdout, "Failed to open the named pipe\n");
            exit(-2);
        }
    }
    int pipe_num_read;
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
    ping_pkt = (struct Ping_Pkt*) (packet);

    ping_msg = (struct Ping_Msg*) (packet);
    ping_pkt->type = CONTROL; //prepare the first packet
    ping_msg->code = CNT_NONE;
    if (setup->getFilename().length() && setup->sendFilename()) {
        strcpy(ping_msg->msg, setup->getFilename().c_str());
        ping_msg->code = CNT_FNAME;
    } else {
        ping_msg->code = CNT_NOFNAME;
    }
    unsigned int pk_len = 16 + strlen(ping_msg->msg); //fixed header len of ping msg=16
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
        if (timeout == 10) {
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
    u_int64_t speed_int = this->getInterval();
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

    cinterval = speed_int * 1000L;
    if (cinterval + tt.tv_nsec > 999999999) {
        req.tv_sec = tt.tv_sec + 1;
        req.tv_nsec = ((cinterval + tt.tv_nsec) - 1000000000);
    } else {
        req.tv_sec = tt.tv_sec;
        req.tv_nsec = ((cinterval + tt.tv_nsec));
    }

    for (int i = 1; i <= setup->getCount(); i++) {
        ping_pkt->seq = i;

        timeval ts;
        gettimeofday(&ts, NULL);
        ping_pkt->sec = ts.tv_sec;
        ping_pkt->usec = ts.tv_usec;

        //deadline reached [ -w ];
        if (stop) break;
        pkt_sent++;
        pipe_num_read = payload_size = this->getPacketSize() - HEADER_LENGTH;
        if (setup->npipe()) {
            pipe_num_read = read(pipe_handle, pipe_buffer, payload_size);
            if (!pipe_started) {
                if (pipe_num_read == 0) {
                    usleep(200000);
                    pipe_cnt++;
                    if (pipe_cnt++ < 600) continue; //2 minutes timeout.
                } else {
                    pipe_started = true;
                    i = 1;
                    pkt_sent = 1;
                }
            }
            if (pipe_num_read > 0) {
                memcpy(ping_pkt->padding, pipe_buffer, pipe_num_read);
            }
            if (pipe_num_read < payload_size) {
                stop = true;
            }
        }
        nRet = sendto(this->sock, packet, HEADER_LENGTH + pipe_num_read, 0, (struct sockaddr *) &saServer, sizeof (struct sockaddr));
        if (nRet < 0) {
            cerr << "Packet size:" << HEADER_LENGTH + pipe_num_read << endl;
            perror("sending");
            close(this->sock);
            exit(1);
        }
        if (speedup) {
            interval = speed_int;
        } else {
            interval = this->getInterval();
            //cout << "Interval: " << interval << endl;
        }
        if (setup->actWaiting()) {
            gettimeofday(&curTv, NULL);
            u_int64_t curTime = curTv.tv_usec + curTv.tv_sec * 1000000;
            u_int64_t tgTime = curTime + interval + correction;
            while (curTime < tgTime) {
                gettimeofday(&curTv, NULL);
                curTime = curTv.tv_usec + curTv.tv_sec * 1000000;
            }
            gettimeofday(&curTv, NULL);
            delta = ((double) (curTv.tv_sec - refTv.tv_sec)*1000.0 + (double) (curTv.tv_usec - refTv.tv_usec) / 1000.0);
            gettimeofday(&refTv, NULL);
            correction = (int) ((interval + correction) - delta * 1000);
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
            gettimeofday(&refTv, NULL);
            correction = (int) ((interval + correction) - delta);
            if (-correction > interval - 1) {
                correction = -(interval - 1);
            }
            cinterval = (interval + correction) * 1000L;
            if (cinterval + tt.tv_nsec > 999999999) {
                req.tv_sec = tt.tv_sec + 1;
                req.tv_nsec = ((cinterval + tt.tv_nsec) - 1000000000);
            } else {
                req.tv_sec = tt.tv_sec;
                req.tv_nsec = ((cinterval + tt.tv_nsec));
            }
        }
        if (setup->showSendBitrate()) {
            ss.str("");
            if (setup->showTimeStamps()) {
                sprintf(msg, "[%d.%06d] ", ts.tv_sec, ts.tv_usec);
                ss << msg;
            }
            sprintf(msg, "%d bytes to %s: req=%d delta=%.2f ms tx_rate=%.2f kbit/s \n", nRet, setup->getHostname().c_str(), ping_pkt->seq, delta / 1000, (1000 / delta) * nRet * 8);
            ss << msg;
            fprintf(fp, "%s", ss.str().c_str());
        }
    }
    ping_pkt->type = CONTROL;
    ping_msg->code = CNT_DONE;
    timeout = 0;
    while (!done) {
        nRet = sendto(this->sock, packet, pk_len, 0, (struct sockaddr *) &saServer, sizeof (struct sockaddr));
        if (nRet < 0) {
            perror("sending");
            close(this->sock);
            exit(1);
        }
        usleep(200000); //  5pkt/s
        timeout++;
        if (timeout == 10) {
            printf("Can't get stats from server\n");
            exit(1);
        }
    }
    close(pipe_handle);
#ifdef DEBUG
    cout << setup->getInterval() << endl;
#endif    

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
#ifdef DEBUG
        if (setup->debug()) cout << "-D- * T1 * ";
#endif
        interval = this->base_interval;
        //cout << "Bitrate:" << setup->getBaseRate() << endl;
    } else {

        if (setup->shape()) {
            //cout << "Shape definition in use" << endl;
            gettimeofday(&cur_ts, NULL);
            time = ((cur_ts.tv_sec * 1000000 + cur_ts.tv_usec)-(this->start_ts.tv_sec * 1000000 + this->start_ts.tv_usec));
            double tmp1 = setup->getPacketSize()*8000000;
            double tmp2 = setup->getRTBitrate(time);
            interval = (u_int64_t) (tmp1 / tmp2);
            //cout << "Stage 1 - inteval:" << interval << endl;
        } else {
            //cout << "Shape definition NOT in use" << endl;
            if (time<this->t1) {
                interval = this->base_interval;
            } else {
                gettimeofday(&cur_ts, NULL);
                time = ((cur_ts.tv_sec * 1000000 + cur_ts.tv_usec)-(this->start_ts.tv_sec * 1000000 + this->start_ts.tv_usec)) % this->t2;
                double tmp1 = setup->getPacketSize()*8000000;
                double tmp2 = setup->getBaseRate()+(time - this->t1) * this->bchange;
                interval = (u_int64_t) (tmp1 / tmp2);
                //cout << "\e[0;31m DEBUG \e[1;37m INTERVAL = " << interval << " TMP1 = " << tmp1 << " TMP2 = " << tmp2 << " TIME = " << time << " BRATE  = " << setup->getBaseRate() << " BCHANGE = " << this->bchange << "\e[0m" << endl;
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
    u_int16_t payload_size = setup->getPayloadSize();
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
    //cout << "Payload size: " << payload_size << endl;
    return payload_size;
}

/*
void cClient::delay(u_int64_t usec) {
    req.tv_sec = usec / 1000000L;
    req.tv_nsec = usec * 1000L;
    while (nanosleep(&req, &rem) == -1) {
        if (errno == EINTR) {
            req = rem;
            cout << "Interupt correction" << endl;
        }
    }
}
 */


void cClient::delay(timespec req) {
    int e = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &req, &req);
}


/* Radove mene presne nez clock_nanosleep
void cClient::delay(u_int64_t usec) {
    struct timeval tv;
    fd_set dummy;
    int s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    FD_ZERO(&dummy);
    FD_SET(s, &dummy);
    tv.tv_sec = usec / 1000000L;
    tv.tv_usec = usec % 1000000L;
    bool sucess = 0 == select(0, 0, 0, &dummy, &tv);
    close(s);
}
 */