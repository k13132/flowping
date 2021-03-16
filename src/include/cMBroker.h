//
// Created by Ondřej Vondrouš on 2019-02-20.
//

#ifndef FLOWPING_CMESSAGEBROKER_H
#define FLOWPING_CMESSAGEBROKER_H

#include "_types.h"
#include "cSetup.h"
#include "cStats.h"
#include "SPSCQueue.h"



class cMessageBroker {
public:
    cMessageBroker(cSetup *setup, cStats *stats);
    virtual ~cMessageBroker();
    void push(t_conn * conn, gen_msg_t *msg);
    void push_rx(gen_msg_t *msg);
    void push_tx(gen_msg_t *msg);
    void push_lp(gen_msg_t *msg);
    void push_hp(gen_msg_t *msg);
    void run();
private:
    // todo remove dup
    u_int64_t dup;
    u_int64_t dcnt, dcnt_rx, dcnt_tx;
    struct timespec curTv;
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;

    cSetup *setup;
    cClientStats *c_stats;
    cServerStats *s_stats;

    sampled_int_t sampled_int[2];


    struct ping_pkt_t *ping_pkt;
    struct ping_msg_t *ping_msg;
    rigtorp::SPSCQueue<t_msg_t*> msg_buf {128000};
    rigtorp::SPSCQueue<t_msg_t*> msg_buf_rx {128000};
    rigtorp::SPSCQueue<t_msg_t*> msg_buf_tx {128000};
    rigtorp::SPSCQueue<t_msg_t*> msg_buf_lp {64};
    rigtorp::SPSCQueue<t_msg_t*> msg_buf_hp {64};
    void processAndDeleteClientMessage(t_msg_t *tmsg);
    void processAndDeleteServerMessage(t_msg_t *tmsg);
    std::string prepHeader();
private:
    bool json_first;
    std::string prepDataRec(const u_int64_t ts, const u_int8_t dir, const uint16_t size, const uint64_t seq, const float rtt);
    std::string prepFinalDataRec(const u_int8_t dir);
private:
    std::ofstream fout;
    std::ostream* output;
private:
    float jt_rtt = 0; //ms
    float jt_rtt_prev = 0; //ms
    float jitter = 0; //ms
    float jt_diff = 0; //ms
    float pkt_rtt;
    float rtt_min, rtt_max, rtt_avg;
    u_int64_t time, pkt_sent, server_received, pkt_rcvd, last_seq_rcv, ooo_cnt, dup_cnt;

};


#endif //FLOWPING_CMESSAGEBROKER_H
