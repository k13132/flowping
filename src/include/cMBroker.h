//
// Created by Ondřej Vondrouš on 2019-02-20.
//

#ifndef FLOWPING_CMESSAGEBROKER_H
#define FLOWPING_CMESSAGEBROKER_H

#include "_types.h"
#include "cSetup.h"
#include "SPSCQueue.h"
#include "concurrentqueue.h"

class cMessageBroker {
public:
    cMessageBroker(cSetup *setup);
    virtual ~cMessageBroker();
    void push(gen_msg_t *msg);
    void run();
private:
    // todo remove dup
    cSetup *setup;
    //cMessageBroker *mbroker;
    //struct ping_pkt_t *ping_pkt;
    struct ping_msg_t *ping_msg;
    moodycamel::ConcurrentQueue<t_msg_t*> msg_buf {64000};


    void processMessage(u_int64_t  key, gen_msg_t *msg);
};


#endif //FLOWPING_CMESSAGEBROKER_H
