//
// Created by Ondřej Vondrouš on 22.11.2021.
//

#include "cSlotTimer.h"

cSlotTimer::cSlotTimer(cMessageBroker *broker, cSetup *setup){
    this->setup = setup;
    this->mbroker = broker;
    this->run_active = false;
    this->timerEnabled = false;
    if (setup->isClient()){
        if (setup->getSampleLen()){
            this->timerEnabled = true;
            this->slots[0].interval = std::chrono::nanoseconds(setup->getSampleLen());
            this->slots[0].trigger = std::chrono::system_clock::now();
        }
    }
    if (setup->isServer()){
        this->timerEnabled = true;
    }
}

cSlotTimer::~cSlotTimer() {

}

void cSlotTimer::start() {
    if (setup->isClient()){
        if (setup->getSampleLen()){
            run_active = true;
            this->slots[0].trigger = std::chrono::system_clock::now();
            this->slots[0].trigger += this->slots[0].interval;
        }
    }
    msg = new gen_msg_t;
    msg->type = MSG_SLOT_TIMER_START;
    mbroker->push_hp(msg);
}

//can not be started again after stop is called!
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
            //client with -L > 0?
            if (setup->isClient()){
                if (setup->getSampleLen()){
                    if (tp_slot_current >= this->slots[0].trigger){
                        this->slots[0].trigger += this->slots[0].interval;
                        msg = new gen_msg_t;
                        msg->type = MSG_SLOT_TIMER_TICK;
                        mbroker->push_hp(msg);
                    }
                }
            }
            if (setup->isServer()){
                for (auto& [flow_id, slot] : this->slots){
                    if (tp_slot_current >= slot.trigger){
                        slot.trigger += slot.interval;
                        msg = new gen_msg_t;
                        msg->type = MSG_SLOT_TIMER_TICK;
                        msg->flow_id = flow_id;
                        mbroker->push_hp(msg, slot.conn);
                    }
                }
            }
        }
        usleep(100);
    }
}

void cSlotTimer::addTimer(uint16_t flowid, uint64_t slot_interval, t_conn * conn) {
    if (slot_interval){
        std::chrono::nanoseconds interval(slot_interval);
        slot_t slot;
        slot.interval = interval;
        slot.trigger = std::chrono::system_clock::now();
        slot.trigger += interval;
        slot.conn = conn;
        this->slots[flowid] = slot;
        this->run_active = true;
    }
}

void cSlotTimer::removeTimer(uint16_t flow_id) {
    if (this-slots.count(flow_id)){
        this->slots.erase(flow_id);
    }
    if (this->slots.size()<1){
        this->run_active = false;
    }
}
