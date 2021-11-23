/*
 * File:   _types.h
 *
 * Copyright (C) 2017: Department of Telecommunication Engineering, FEE, CTU in Prague
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


#ifndef _TYPES_H
#define	_TYPES_H

//#include <cstdlib>
#include <stdint.h>
#include <string>
#include <ctime>
#include <chrono>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <climits>
#include <cstdlib>
#include <getopt.h>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <queue>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <ostream>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <cerrno>
#include <cmath>
#include <map>

#define MAX_PKT_SIZE 1472
#define MIN_PKT_SIZE 32  //we need to transfer whole header.
#define HEADER_LENGTH 32

#define CONTROL 0
#define PING 32
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
#define CNT_JPAR 128  //1000 0000       


#define MSG_RX_CNT 0
#define MSG_TX_CNT 1
#define MSG_RX_PKT 32
#define MSG_TX_PKT 33
#define MSG_KEEP_ALIVE 64
#define MSG_OUTPUT_INIT 65
#define MSG_OUTPUT_CLOSE 66
#define MSG_SLOT_TIMER_START 90
#define MSG_SLOT_TIMER_STOP 91
#define MSG_SLOT_TIMER_TICK 92
#define MSG_TIMER_ONE 99
#define MSG_TIMER_END 100
#define MSG_SOCK_TIMEOUT 110


#define TX 0
#define RX 1

#define TV_SEC(ts) ( ((ts) / 1000000000L) )
#define TV_NSEC(ts) ( ((ts) % 1000000000L) )
#define TV_USEC(ts) ( ((ts) % 1000000000L)/1000L )
#define NS_TDIFF(tv1,tv2) ( ((tv1).tv_sec-(tv2).tv_sec)*1000000000 +  ((tv1).tv_nsec-(tv2).tv_nsec) )
#define NS_TIME(tv1) ( (tv1).tv_sec*1000000000 +  (tv1).tv_nsec )
#define max(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })


using namespace std;

struct event_t{
    timespec ts;
    std::string msg;
};


//struct ping_pkt_t {         //Min PK SIZE 32B
//    u_int8_t type;
//    u_int8_t id;
//    u_int16_t size;         //payload_size
//    u_int16_t flow_id;
//    u_int16_t padding1;
//    u_int64_t sec;
//    u_int64_t nsec;
//    u_int64_t seq;
//    char data[MAX_PKT_SIZE-HEADER_LENGTH];
//};

struct ping_pkt_t {         //Header size 32B 1+1+2+2+4+4+4+4+8+2, Min. Payload size = 0B
    u_int8_t type;
    u_int8_t id;
    u_int16_t size;         //payload_size
    u_int16_t flow_id;
    u_int16_t padding1;
    u_int32_t sec;
    u_int32_t nsec;
    u_int32_t server_sec;
    u_int32_t server_nsec;
    u_int64_t seq;
    char data[MAX_PKT_SIZE-HEADER_LENGTH];
};

struct ping_msg_t {         //Min MSG SIZE 16B
    u_int8_t type;
    u_int8_t code;
    u_int16_t size;
    u_int64_t count;
    u_int8_t params;        // 00000001 - H_PAR //Bit encoded
    u_int8_t id;
    u_int16_t check;
    uint64_t padding1;
    uint64_t padding2;
    char msg[MAX_PKT_SIZE-HEADER_LENGTH];
};

struct gen_msg_t{           //Min MSG SIZE 4B
    u_int8_t type;
    u_int8_t id;
    u_int16_t size;
    u_int64_t padding_3;
    u_int64_t padding_4;
    u_int64_t padding_5;
    char msg[MAX_PKT_SIZE-HEADER_LENGTH];
};

struct t_conn{
    timespec  curTv, refTv;
    u_int64_t pkt_cnt;
    in6_addr ip;
    u_int16_t port;
    u_int64_t conn_id;
    u_int32_t ret_size;
    string client_ip;
    bool C_par;
    bool J_par;
    bool D_par;
    bool e_par;
    bool E_par;
    bool F_par;
    bool H_par;
    bool X_par;
    bool AX_par;
    bool W_par;
    std::ofstream fout;
};


struct t_msg_t{
    std::chrono::system_clock::time_point tp;
    gen_msg_t* msg;
    t_conn* conn;
};


struct tpoint_def_t{
    double ts;
    double bitrate;
    unsigned int len;
};

struct timed_packet_t{
    u_int64_t ts;
    u_int32_t sec;
    u_int32_t nsec;
    u_int16_t len;
};

struct sampled_int_t{
    bool first;
    double rtt_sum;
    u_int64_t first_seq;
    u_int64_t last_seen_seq;
    u_int64_t pkt_cnt;
    u_int64_t bytes;
    u_int64_t ts_limit;
    u_int64_t seq;
    u_int64_t ooo;
    u_int64_t dup;
    float jitter_sum;
};



#endif	/* _TYPES_H */

