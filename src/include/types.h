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


#ifndef xTYPES_H
#define	xTYPES_H

#include <stdint.h>
#include <string>
#include <ctime>
#include <chrono>
#include <sys/types.h>
#include <sys/stat.h>
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

#define MAX_PAYLOAD_SIZE 1472
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
#define CNT_LPAR 2  //0000 0010
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
//    uint8_t type;
//    uint8_t id;
//    uint16_t size;         //payload_size
//    uint16_t flow_id;
//    uint16_t padding1;
//    uint64_t sec;
//    uint64_t nsec;
//    uint64_t seq;
//    char data[MAX_PAYLOAD_SIZE-HEADER_LENGTH];
//};

//Do not pack this structure!
struct ping_pkt_t {         //Header size 32B 1+1+2+2+4+4+4+4+8+2, Min. Payload size = 0B
    uint8_t type;
    uint8_t id;
    uint16_t size;         //payload_size
    uint16_t flow_id;
    uint16_t padding1;
    uint32_t sec;
    uint32_t nsec;
    uint32_t server_sec;
    uint32_t server_nsec;
    uint64_t seq;
    char data[MAX_PAYLOAD_SIZE];
};

struct __attribute__ ((packed)) ping_msg_t {         //Min MSG SIZE 16B 1+1+2+8+1+1+2+8+8
    uint8_t type;
    uint8_t code;
    uint16_t size;
    uint16_t flow_id;
    uint8_t params;        // 00000001 - H_PAR //Bit encoded
    uint8_t id;
    uint16_t check;
    uint16_t padding1;
    uint32_t sample_len_ms;
    uint64_t count;
    char msg[MAX_PAYLOAD_SIZE];
};

// struct
// because of alignment it is necessary to have packed structure or use exact same padding/structure as ping_msg_t
//packed structure is use only in setup phase so no need to worry about performance penalty.
struct __attribute__ ((packed)) gen_msg_t{           //Min MSG SIZE 4B
    uint8_t type;
    uint8_t id;
    uint16_t size;
    uint16_t flow_id;
    uint8_t padding_1;
    uint8_t padding_2;
    uint16_t padding_3;
    uint16_t padding_4;
    uint32_t padding_5;
    uint64_t padding_6;
    char msg[MAX_PAYLOAD_SIZE]; //offset must be same as offset of msg in ping_msg_t
};


struct sampled_int_t{
    bool first;
    double rtt_sum;
    double delay_sum;
    uint64_t first_seq;
    uint64_t last_seen_seq;
    uint64_t pkt_cnt;
    uint64_t bytes;
    uint64_t ts_limit;
    uint64_t seq;
    uint64_t ooo;
    uint64_t dup;
    float jitter_sum;
};


struct t_conn{
    std::chrono::system_clock::time_point start, end, last_pkt_rcvd;
    timespec  curTv, refTv;
    uint64_t pkt_rx_cnt;
    uint64_t pkt_tx_cnt;
    uint64_t bytes_rx_cnt;
    uint64_t bytes_tx_cnt;
    float jitter;
    float jt_prev;
    float jt_diff;
    float jt_delay_prev;
    in6_addr ip;
    sa_family_t family;
    uint16_t port;
    uint64_t conn_id;
    uint16_t size;
    uint16_t ret_size;
    uint64_t sample_len;
    string client_ip;
    sampled_int_t sampled_int[2];
    uint64_t dup_cnt;
    uint64_t ooo_cnt;
    bool finished;
    bool initialized;
    bool started;
    bool C_par;
    bool J_par;
    bool D_par;
    bool e_par;
    bool E_par;
    bool F_par;
    bool H_par;
    bool X_par;
    bool L_par;
    bool AX_par;
    std::ofstream fout;
    std::ostream* output;
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
    uint64_t ts;
    uint32_t sec;
    uint32_t nsec;
    uint16_t len;
};



#endif	/* _TYPES_H */

