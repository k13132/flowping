//
// Created by Ondřej Vondrouš on 22.11.2021.
//

#ifndef FLOWPING_CSLOTTIMER_H
#define FLOWPING_CSLOTTIMER_H

#include "types.h"
#include "cSetup.h"
#include "cMBroker.h"

class cSlotTimer {
public:
    cSlotTimer(cMessageBroker *, cSetup *);
    virtual ~cSlotTimer();
    void start();
    void stop();
    void run();
private:
    bool run_active, timerEnabled;
    std::chrono::nanoseconds slot_interval;
    cMessageBroker* mbroker;
    cSetup* setup;
    gen_msg_t* msg;
    std::chrono::system_clock::time_point tp_start,tp_slot_trigger, tp_slot_current;
};


#endif //FLOWPING_CSLOTTIMER_H
