//
// Created by Ondřej Vondrouš on 2019-02-20.
//

#include "cMBroker.h"
#include "cSetup.h"
#include <chrono>


std::ostream& operator<<(std::ostream& os, const ping_pkt_t& obj)
{
    os << obj.size + HEADER_LENGTH << " bytes, req=" << obj.seq;
    return os;
}

std::ostream& operator<<(std::ostream& os, const ping_msg_t& obj)
{
    os << "type: " << (u_int16_t)obj.type << " / code: " << (u_int16_t)obj.code << " msg:" << obj.msg;
    return os;
}



cMessageBroker::cMessageBroker(cSetup *setup){
    dup = 0;
    if (setup) {
        this->setup = setup;
    }else{
        //todo KILL execution, we can not continue without setup module;
        this->setup = nullptr;
    }
    dcnt_rx = 0;
    dcnt_tx = 0;
}


cMessageBroker::~cMessageBroker(){

}


void cMessageBroker::push_rx(gen_msg_t *msg){
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    key_rx = tp.time_since_epoch().count();
    msg_buf_mutex_rx.lock();
    msg_buf_rx[key_rx] = msg;
    msg_buf_mutex_rx.unlock();
    dcnt_rx++;
}

void cMessageBroker::push_tx(gen_msg_t *msg){
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    key_tx = tp.time_since_epoch().count();
    msg_buf_mutex_tx.lock();
    msg_buf_tx[key_tx] = msg;
    msg_buf_mutex_tx.unlock();
    dcnt_tx++;
}

void cMessageBroker::d_print() {
    msg_buf_mutex_tx.lock();
    msg_buf_mutex_rx.lock();
    std::cout << "buffer TX contains: " << msg_buf_tx.size() << " records" << std::endl;
    std::cout << "buffer RX contains: " << msg_buf_rx.size() << " records" << std::endl;
    msg_buf_mutex_tx.unlock();
    msg_buf_mutex_rx.unlock();
}

void cMessageBroker::run(){
    //while ((not setup->isDone()) && (not setup->isStop())){
    u_int64_t key_rx;
    u_int64_t key_tx;
    u_int64_t key;
    gen_msg_t * msg;
    u_int64_t first=0;
    u_int64_t last=0;
    while (!setup->isDone()){
        usleep(3000);
        while ((not msg_buf_rx.empty())||(not msg_buf_tx.empty())){
            key_rx = 0;
            key_tx = 0;
            if (not msg_buf_rx.empty()){
                key_rx= msg_buf_rx.begin()->first;
            }
            if (not msg_buf_tx.empty()){
                key_tx= msg_buf_tx.begin()->first;
            }
            if ((key_rx < key_tx) || ((key_tx)&&(not key_rx))){
                msg_buf_mutex_tx.lock();
                msg = msg_buf_tx.begin()->second;
                msg_buf_tx.erase(key_tx);
                msg_buf_mutex_tx.unlock();
                if (first==0) first = key_tx;
                last = key_tx;
                processMessage(key_tx,msg); //msg is deleted at this point
            }
            if ((key_rx >= key_tx) || ((not key_tx)&&(key_rx))){
                msg_buf_mutex_rx.lock();
                msg = msg_buf_rx.begin()->second;
                msg_buf_rx.erase(key_rx);
                msg_buf_mutex_rx.unlock();
                processMessage(key_rx,msg); //msg is deleted at this point
            }
        }
    }
    std::cerr << "MSG processed TX/RX: " << dcnt_tx << "/" << dcnt_rx << std::endl;
    std::cerr << "Packet generator was active for [sec]:  " << (last-first)/1000000000.0 << std::endl;
    std::cerr << "Key collisions:  " << dup << std::endl;
}

void cMessageBroker::processMessage(u_int64_t  key, gen_msg_t * msg){
    //todo refactor - remove nRet
    u_int16_t nRet = 0;
    if (msg == nullptr){
        std::cout << "MSG_NULL" << std::endl;
        return;
    }
    switch(msg->type){
        case MSG_RX_CNT:
            //Todo modify structure !!!! packet data not present - ONLY header was copied
            ping_msg = (struct ping_msg_t*) (msg);
            std::cout << "MSG_RX_CNT:" << *ping_msg << std::endl;
            if (ping_msg->code == CNT_DONE_OK) {
                setup->setDone(true);
                std::cout << "done" << std::endl;
            }
            if (ping_msg->code == CNT_FNAME_OK) setup->setStarted(true);
            if (ping_msg->code == CNT_OUTPUT_REDIR) setup->setStarted(true);
            break;
        case MSG_TX_CNT:
            std::cout << "MSG_TX_CNT" << std::endl;
            break;
        case MSG_RX_PKT:
            //std::cout << "MSG_RX_PKT" << std::endl;
            //Todo modify structure !!!! packet data not present - ONLY header was copied
            ping_pkt = (struct ping_pkt_t*) (msg);
            //std::cerr << *ping_pkt << std::endl;
            break;
        case MSG_TX_PKT:
            //std::cout << "MSG_TX_PKT" << std::endl;
            break;
        case MSG_KEEP_ALIVE:
            std::cout << "MSG_KEEP_ALIVE" << std::endl;
            break;
        default:
            std::cerr << "ERROR :: UNKNOWN MSG TYPE RECEIVED!";
    }
    delete(msg);
    msg = nullptr;
};



