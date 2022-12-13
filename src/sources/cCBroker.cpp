//
// Created by Ondřej Vondrouš on 12.12.2022.
//

#include "cCBroker.h"


cConnectionBroker::cConnectionBroker(cMessageBroker *mbroker) {
    this->mbroker = mbroker;
    this->gc_counter = 0;
    this->max_connections = 64;
    this->active = true;
}

cConnectionBroker::~cConnectionBroker() {

}

string stripFFFF(string str) {
    if (str.find("::ffff:",0,7) == 0) return str.substr(7);
    return str;
}

conn_t *cConnectionBroker::getConn(sockaddr_in6 saddr, ping_pkt_t *pkt, bool is_asym){
    conn_t * connection = nullptr;
    if (connections.size() > this->max_connections){
        return nullptr;
    }
    uint64_t conn_id = (uint64_t) pkt->flow_id;
    if (this->connections.count(conn_id) == 1) {
        connection = this->connections.at(conn_id);
        if (is_asym){
            connection->ret_size = MIN_PKT_SIZE;
        }else{
            connection->ret_size = pkt->size;
        }
    } else {
        //std::cout << "Initializing connection with id: " << conn_id << std::endl;
        char addr[INET6_ADDRSTRLEN];
        connection = new conn_t;
        connection->saddr = saddr;
        connection->ip = saddr.sin6_addr;
        connection->port = saddr.sin6_port;
        connection->family = saddr.sin6_family;
        connection->conn_id = conn_id;
        connection->sample_len = 0;
        inet_ntop(AF_INET6, &saddr.sin6_addr, addr, INET6_ADDRSTRLEN);
        connection->client_ip = stripFFFF(string(addr));
        connection->pkt_tx_cnt = 0;
        connection->pkt_rx_cnt = 0;
        connection->bytes_tx_cnt = 0;
        connection->bytes_rx_cnt = 0;
        connection->refTv.tv_sec = 0;
        connection->refTv.tv_nsec = 0;
        connection->curTv.tv_sec = 0;
        connection->curTv.tv_nsec = 0;
        connection->jitter = 0;
        connection->jt_prev = 0;
        connection->jt_diff = 0;
        connection->jt_delay_prev = 0;
        connection->finished = false;
        connection->initialized = false;
        connection->started = false;
        connection->C_par = false;
        connection->D_par = false;
        connection->e_par = false;
        connection->E_par = false;
        connection->F_par = false;
        connection->H_par = false;
        connection->J_par = false;
        connection->L_par = false;
        connection->X_par = is_asym;
        connection->AX_par = false;
        connection->size = HEADER_LENGTH;
        connection->ret_size = HEADER_LENGTH;
        for (int i = 0; i < 2; i++){
            connection->sampled_int[i].first = true;
            connection->sampled_int[i].first_seq = 0;
            connection->sampled_int[i].pkt_cnt = 0;
            connection->sampled_int[i].ts_limit = 0;
            connection->sampled_int[i].seq = 0;
            connection->sampled_int[i].ooo = 0;
            connection->sampled_int[i].dup = 0;
            connection->sampled_int[i].rtt_sum = 0;
            connection->sampled_int[i].bytes = 0;
            connection->sampled_int[i].jitter_sum = 0;
            connection->sampled_int[i].last_seen_seq = 0;
        }
        this->connections[conn_id] = connection;
    }
    return connection;
}

void cConnectionBroker::run() {
    while (active){
        gc(false);
        usleep(100000); //100ms
    }
}

void cConnectionBroker::gc(bool force) {
    if (force || (gc_counter > 100)){
        clock_gettime(CLOCK_REALTIME, &tv);
        vector<uint64_t> keys = {};
        for (const auto & [key, conn] : connections) {
            if (conn == nullptr){
                //std::cerr << "deleting NULL connection with id: " << key << std::endl;
                keys.push_back(key);
                continue;
            }
            if (conn->finished){
                //std::cerr << "deleting FINISHED connection with id: " << key << std::endl;
                keys.push_back(key);
                continue;
            }
            if ((conn->curTv.tv_sec + 3600) < tv.tv_sec){
                std::cerr << "removing TIMEOUTED connection with flow_id: " << key << std::endl;
                tmsg = new gen_msg_t;
                tmsg->type = MSG_OUTPUT_CLOSE;
                mbroker->push((gen_msg_t*)tmsg, conn);
                //keys.push_back(key);
                continue;
            }
        }
        while (!keys.empty()){
            connections.erase(keys.back());
            keys.pop_back();
        }
        gc_counter = 0;
    }
    gc_counter ++;
}

vector<uint64_t> cConnectionBroker::getActiveConnIDs() {
    vector<uint64_t> ids = {};
    this->gc(false);
    for (const auto & [key, conn] : connections) {
        ids.push_back(key);
    }
    return ids;
}

uint16_t cConnectionBroker::getConnCount() const {
    return connections.size();
}

conn_t *cConnectionBroker::getConnByID(uint64_t conn_id) const {
    if (this->connections.count(conn_id) == 1) {
        return connections.at(conn_id);
    }
    return nullptr;
}

void cConnectionBroker::stop() {
    this->active = false;
}



