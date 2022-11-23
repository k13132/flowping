//
// Created by Ondřej Vondrouš on 22.11.2021.
//

#ifndef FLOWPING_CSLOTTIMER_H
#define FLOWPING_CSLOTTIMER_H

#include "types.h"
#include "cSetup.h"
#include "cMBroker.h"

struct slot_t{
    std::chrono::system_clock::time_point trigger;
    std::chrono::nanoseconds interval;
    t_conn * conn;
};


class cSlotTimer {
public:
    cSlotTimer(cMessageBroker *, cSetup *);
    virtual ~cSlotTimer();
    void start();
    void addTimer(uint16_t flow_id, uint64_t slot_interval, t_conn *conn);
    void removeTimer(uint16_t flow_id);
    void stop();
    void run();
private:
    bool run_active, timerEnabled;
    cMessageBroker* mbroker;
    cSetup* setup;
    gen_msg_t* msg;
    std::chrono::system_clock::time_point tp_start,tp_slot_current;
    std::map<uint16_t, slot_t> slots;
};


#endif //FLOWPING_CSLOTTIMER_H
