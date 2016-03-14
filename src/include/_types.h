/* 
 * File:   _types.h
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

#ifndef _TYPES_H
#define	_TYPES_H


#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <sstream>
#include <climits>
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <queue>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <pthread.h>
#include <errno.h>
#include <math.h>

#define MAX_PKT_SIZE 1476
#define MIN_PKT_SIZE 64  //equal to header size
#define HEADER_LENGTH 32

#define CONTROL 1
#define PING 0
#define CNT_NONE 0
#define CNT_DONE 1
#define CNT_DONE_OK  2
#define CNT_FNAME 3
#define CNT_NOFNAME 4
#define CNT_FNAME_OK 5
#define CNT_OUTPUT_REDIR 6

#define CNT_HPAR 1  //0000 0001
#define CNT_WPAR 2  //0000 0010
#define CNT_CPAR 4  //0000 0100
#define CNT_XPAR 8  //0000 1000
#define CNT_DPAR 16  //0001 0000
#define CNT_ePAR 32  //0010 0000
#define CNT_EPAR 64  //0100 0000

#if __WORDSIZE == 64
#define xENV_64
#else
#define xENV_32
#endif

struct ts_t{
    u_int64_t sec;
    u_int64_t nsec;
};

struct event_t{
    ts_t ts;
    std::string msg;
};


struct ping_pkt_t {         //Min PK SIZE 32B
    u_int8_t type;
    u_int8_t id;
    u_int16_t size;         //payload_size
    u_int16_t padding1;
    u_int16_t check;
    u_int64_t sec;
    u_int64_t nsec;
    u_int64_t seq;
    char padding[MAX_PKT_SIZE];
};

struct ping_msg_t {         //Min MSG SIZE 16B
    u_int8_t type;
    u_int8_t code;
    u_int64_t count;
    u_int16_t size;
    u_int8_t params;    /// 00000001 - H_PAR //Bit encoded
    u_int8_t id;
    u_int16_t check;
    char msg[MAX_PKT_SIZE];
};

struct tpoint_def_t{
    double ts;
    double bitrate;
    unsigned int len;
};

struct timed_packet_t{
    u_int64_t sec;
    u_int64_t nsec;
    u_int16_t len;
};

#endif	/* _TYPES_H */

