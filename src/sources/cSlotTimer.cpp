//
// Created by Ondřej Vondrouš on 22.11.2021.
//

#include "cSlotTimer.h"
#include "types.h"
#include "cSetup.h"
#include "cMBroker.h"

cSlotTimer::cSlotTimer(cMessageBroker *broker, cSetup *setup){
    this->setup = setup;
    this->mbroker = broker;
    this->run_active = false;
    this->timerEnabled = false;
    if (setup->getSampleLen()){
        this->timerEnabled = true;
    }
    slot_interval = std::chrono::nanoseconds(setup->getSampleLen());
}

cSlotTimer::~cSlotTimer() {

}

void cSlotTimer::start() {
    run_active = true;
    tp_start = std::chrono::system_clock::now();
    tp_slot_trigger = tp_start;
    tp_slot_trigger += slot_interval;
    tp_slot_current = tp_start;
    msg = new gen_msg_t;
    msg->type = MSG_SLOT_TIMER_START;
    mbroker->push_hp(msg);
}

void cSlotTimer::stop() {
    run_active = false;
    timerEnabled = false;
    msg = new gen_msg_t;
    msg->type = MSG_SLOT_TIMER_STOP;
    mbroker->push_hp(msg);
}

void cSlotTimer::run() {
    while(this->timerEnabled){
        tp_slot_current = std::chrono::system_clock::now();
        if (run_active){
            if (tp_slot_current >= tp_slot_trigger){
                tp_slot_trigger +=  slot_interval;
                msg = new gen_msg_t;
                msg->type = MSG_SLOT_TIMER_TICK;
                mbroker->push_hp(msg);
            }
        }
        usleep(100);
    }
}

