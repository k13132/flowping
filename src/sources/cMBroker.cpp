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
    os << "type:" << obj.code << " msg:" << obj.msg;
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
}


cMessageBroker::~cMessageBroker(){

}


void cMessageBroker::push_rx(gen_msg_t *msg){
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    u_int64_t key = tp.time_since_epoch().count();
    msg_buf_mutex_rx.lock();
    while (msg_buf_rx.count(key)>0){
        //std::cout << "duplicate key: " << key << "    " << (u_int16_t)msg_buf[key]->msgType << " vs. " << (u_int16_t)msg->msgType << std::endl;
        key++;
        dup++;
    };
    msg_buf_rx[key] = msg;
    msg_buf_mutex_rx.unlock();
}

void cMessageBroker::push_tx(gen_msg_t *msg){
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    u_int64_t key = tp.time_since_epoch().count();
    msg_buf_mutex_tx.lock();
    while (msg_buf_tx.count(key)>0){
        //std::cout << "duplicate key: " << key << "    " << (u_int16_t)msg_buf[key]->msgType << " vs. " << (u_int16_t)msg->msgType << std::endl;
        key++;
        dup++;
    };
    msg_buf_tx[key] = msg;
    msg_buf_mutex_tx.unlock();
}


void cMessageBroker::run(){
    //while ((not setup->isDone()) && (not setup->isStop())){
    u_int64_t key_rx;
    u_int64_t key_tx;
    u_int64_t key;
    gen_msg_t * msg;
    u_int64_t cnt_rx_pkt = 0;
    u_int64_t cnt_tx_pkt = 0;
    u_int64_t first=0;
    u_int64_t last=0;
    while (!setup->isDone()){
        sleep(30);
        while ((msg_buf_rx.size()>0)||(msg_buf_tx.size()>0)){
            msg_buf_mutex_rx.lock();
            msg_buf_mutex_tx.lock();
            if (msg_buf_rx.size()>0){
                key_rx= msg_buf_rx.begin()->first;
            }else{
                key_rx = 0;
            }
            if (msg_buf_tx.size()>0){
                key_tx= msg_buf_tx.begin()->first;
            }else{
                key_tx = 0;
            }
            if ((key_rx < key_tx) || ((key_tx)&&(not key_rx))){
                msg_buf_mutex_rx.unlock();
                msg = msg_buf_tx.begin()->second;
                msg_buf_tx.erase(key_tx);
                msg_buf_mutex_tx.unlock();
                //std::cout << key_tx << std::endl;
                processMessage(key_tx,msg);
                key = key_tx;
            }
            if ((key_rx > key_tx) || ((not key_tx)&&(key_rx))){
                msg_buf_mutex_tx.unlock();
                msg = msg_buf_rx.begin()->second;
                msg_buf_rx.erase(key_rx);
                msg_buf_mutex_rx.unlock();
                //std::cout << key_rx << std::endl;
                processMessage(key_rx,msg);
                key = key_rx;
            }

            u_int16_t nRet = 0;
            if (msg == nullptr){
                std::cout << "MSG_NULL: " << key << std::endl;
                continue;
            }

            if (msg->msgType == MSG_TX_PKT){
                if (first==0) first = key_rx;
                last = key_rx;
                cnt_tx_pkt++;
            }
            if (msg->msgType == MSG_RX_PKT){
                cnt_rx_pkt++;
            }
        }
    }
    std::cerr << "MSG processed TX/RX: " << cnt_tx_pkt << "/" << cnt_rx_pkt << std::endl;
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
    switch(msg->msgType){
        case MSG_RX_CNT:
            ping_msg = (struct ping_msg_t*) (msg->packet);
            std::cout << "MSG_RX_CNT:" << *ping_msg << std::endl;
            if (ping_msg->code == CNT_DONE_OK) {
                setup->setDone(true);
            }
            if (ping_msg->code == CNT_FNAME_OK) setup->setStarted(true);
            if (ping_msg->code == CNT_OUTPUT_REDIR) setup->setStarted(true);
            break;
        case MSG_TX_CNT:
            std::cout << "MSG_TX_CNT" << std::endl;
            break;
        case MSG_RX_PKT:
            //std::cout << "MSG_RX_PKT" << std::endl;
            ping_pkt = (struct ping_pkt_t*) (msg->packet);
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
};



