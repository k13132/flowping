/* 
 * File:   _types.h
 * 
 * Author: Ondrej Vondrous
 * Email: vondrous@0xFF.cz
 *
 * Created on 27. červen 2012, 8:55
 */

#ifndef _TYPES_H
#define	_TYPES_H

#include <time.h>

#define MAX_PKT_SIZE 1472
#define MIN_PKT_SIZE 24  //equal to header size
#define HEADER_LENGTH 24

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

#if __WORDSIZE == 64
#define xENV_64
#else
#define xENV_32
#endif

struct Ping_Pkt {
    u_int32_t type;
    u_int32_t seq;
    int64_t sec;
    int64_t usec;
    char padding[MAX_PKT_SIZE];
};

struct Ping_Msg {
    u_int32_t type;
    u_int32_t code;
    u_int32_t count;
    u_int8_t params;    /// 00000001 - H_PAR //Bit encoded
    u_int8_t padding1;
    u_int8_t padding2;
    u_int8_t padding3;
    char msg[MAX_PKT_SIZE];
};

struct time_def{
    double ts;
    unsigned int bitrate;
    unsigned int len;
};

#endif	/* _TYPES_H */

