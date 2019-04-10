//
// Created by Ondřej Vondrouš on 2019-02-20.
//

#ifndef FLOWPING_CMESSAGEBROKER_H
#define FLOWPING_CMESSAGEBROKER_H

#include "_types.h"
#include "cSetup.h"
#include "SPSCQueue.h"



class cMessageBroker {
public:
    cMessageBroker(cSetup *setup);
    virtual ~cMessageBroker();
    void push_rx(gen_msg_t *msg);
    void push_tx(gen_msg_t *msg);
    void run();
private:
    // todo remove dup
    u_int64_t dup;
    u_int64_t key_rx, key_tx;
    u_int64_t dcnt_rx, dcnt_tx;
    struct timespec curTv;
    cSetup *setup;
    cMessageBroker *mbroker;
    struct ping_pkt_t *ping_pkt;
    struct ping_msg_t *ping_msg;
    //struct t_msg_t *t_msg;
    SPSCQueue<t_msg_t*> msg_buf_rx {64000};
    SPSCQueue<t_msg_t*> msg_buf_tx {64000};

    void processMessage(u_int64_t  key, gen_msg_t *msg);
};


#endif //FLOWPING_CMESSAGEBROKER_H
