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
    if (setup) {
        this->setup = setup;
    }else{
        //todo KILL execution, we can not continue without setup module;
        this->setup = nullptr;
    }
}


cMessageBroker::~cMessageBroker(){

}


void cMessageBroker::push(gen_msg_t *msg){
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    t_msg_t * t_msg = new t_msg_t;
    t_msg->ts = tp.time_since_epoch().count();
    t_msg->msg = msg;
    msg_buf.enqueue(t_msg);
}

void cMessageBroker::run(){
    t_msg_t * msg;
    while (!setup->isDone()){
        //usleep(30);
        while (msg_buf.try_dequeue(msg)){
            processMessage(msg->ts,msg->msg); //msg is deleted at this point
            delete msg;
        }
    }
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
            //ping_pkt = (struct ping_pkt_t*) (msg);
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
    //std::cout << "MSG delete at: " << msg << std::endl;
    delete(msg);
    msg = nullptr;
};



