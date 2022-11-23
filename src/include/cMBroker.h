//
// Created by Ondřej Vondrouš on 2019-02-20.
//

#ifndef FLOWPING_CMESSAGEBROKER_H
#define FLOWPING_CMESSAGEBROKER_H

#include "types.h"
#include "cSetup.h"
#include "cStats.h"
#include "SPSCQueue.h"



class cMessageBroker {
public:
    cMessageBroker(cSetup *setup, cStats *stats);
    virtual ~cMessageBroker();
    //void push(t_conn * conn, gen_msg_t *msg);
    void push_rx(gen_msg_t *msg);
    void push_tx(gen_msg_t *msg);
    void push(gen_msg_t *msg,t_conn * conn);
    void push_lp(gen_msg_t *msg);
    void push_hp(gen_msg_t *msg);
    void push_hp(gen_msg_t *msg, t_conn * conn);
    void run();
private:
    // todo remove dup
    uint64_t pkt_cnt_tx, pkt_cnt_rx, bytes_cnt_tx, bytes_cnt_rx;
    //uint64_t dcnt, dcnt_rx, dcnt_tx;
    struct timespec curTv;
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;

    cSetup *setup;
    cClientStats *c_stats;
    cServerStats *s_stats;

    sampled_int_t sampled_int[2];

    bool json_first;

    struct ping_pkt_t *ping_pkt;
    struct ping_msg_t *ping_msg;

    rigtorp::SPSCQueue<t_msg_t*> msg_buf {256000};
    rigtorp::SPSCQueue<t_msg_t*> msg_buf_rx {128000};
    rigtorp::SPSCQueue<t_msg_t*> msg_buf_tx {128000};
    rigtorp::SPSCQueue<t_msg_t*> msg_buf_lp {64};
    rigtorp::SPSCQueue<t_msg_t*> msg_buf_hp {64};
    void processAndDeleteClientMessage(t_msg_t *tmsg);
    void processAndDeleteServerMessage(t_msg_t *tmsg);

    std::string prepHeader();
    std::string prepDataRec(const uint64_t ts, const uint64_t pkt_server_ts, const uint8_t dir, const uint16_t size, const uint64_t seq, const float rtt);
    std::string pingOutputRec(const uint64_t ts, const uint64_t pkt_server_ts, const uint8_t dir, const uint16_t size, const uint64_t seq, const float rtt);
    std::string closeDataRecSlot(const uint64_t ts, const uint8_t dir);
    std::string prepFinalDataRec(const uint64_t ts, const uint8_t dir);

    std::string prepServerHeader(t_conn * conn);
    std::string prepServerDataRec(t_msg_t * tmsg, const uint8_t dir);
    std::string closeServerDataRecSlot(const uint64_t ts, t_msg_t * tmsg, const uint8_t dir);
    std::string prepServerFinalDataRec(const uint64_t ts, t_msg_t * tmsg, const uint8_t dir);
    std::ofstream fout;
    std::ostream* output;
    uint64_t last_seen_seq = 0;
    float jt_rtt = 0; //ms
    float jt_rtt_prev = 0; //ms
    float jitter = 0; //ms
    float jitter_sum = 0; //ms
    float jt_diff = 0; //ms
    float jt_prev = 0; //ms
    float pkt_rtt;
    double rtt_sum;
    float rtt_min;
    float rtt_avg;
    float rtt_max;
    uint64_t time, pkt_sent, server_received, pkt_rcvd, ooo_cnt, dup_cnt;
private:
    uint64_t timer_slot_interval;
};


#endif //FLOWPING_CMESSAGEBROKER_H
