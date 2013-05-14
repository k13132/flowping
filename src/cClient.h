/* 
 * File:   client.h
 *
 * Author: Ondrej Vondrous
 * Email: vondrous@0xFF.cz
 * 
 * Created on 26. červen 2012, 22:37
 */

#ifndef CLIENT_H
#define	CLIENT_H

#include "cSetup.h"
#include "_types.h"
#include "uping.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>






class cClient {
public:
    cClient(cSetup *setup);
    int run_sender(void);
    int run_receiver(void);
    void report(void);
    void terminate(void);
    virtual ~cClient();
    
private:
    void delay(struct timespec);
    cSetup *setup;
    u_int64_t getInterval(void);
    u_int16_t getPacketSize(void);
    pthread_t t_sender,t_receiver; 
    bool started,stop,done;
    int sock;
    struct sockaddr_in saServer;
    FILE * fp;
    u_int64_t time,pkt_sent,server_received,pkt_rcvd,last_seq_rcv,ooo_cnt;
    struct timeval start_ts, my_ts,refTv,rrefTv,refTv2,curTv,tmpTv;
    struct timespec req,rem;
    double delta,delta2,delta3;
    double sent_ts,tSent;
    double interval_i, interval_I;
    pthread_barrier_t barr;
    float rtt_min, rtt_max,rtt_avg;
    u_int64_t t1,t2,t3;
    double bchange;          //interval change - bitrate change
    u_int16_t frame_size;
    u_int64_t base_interval;
    u_int64_t interval,min_interval,max_interval;
    
};

#endif	/* CLIENT_H */

