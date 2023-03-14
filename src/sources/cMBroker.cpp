//
// Created by Ondřej Vondrouš on 2019-02-20.
//

#include "cMBroker.h"
#include <chrono>
#include <iomanip>

std::ostream& operator<<(std::ostream& os, const ping_pkt_t& obj)
{
    os << obj.size + HEADER_LENGTH << " bytes, req=" << obj.seq;
    return os;
}

std::ostream& operator<<(std::ostream& os, const ping_msg_t& obj)
{
    os << "type: " << (uint16_t)obj.type << " / code: " << (uint16_t)obj.code << " msg:" << obj.msg;
    return os;
}

//ToDo
std::ostream& operator<<(std::ostream& os, const conn_t& obj)
{
    os << "Connection id: " << (uint16_t)obj.conn_id << std::endl;
    os << "Start ts: " << (uint64_t)obj.start.time_since_epoch().count() << std::endl;
    os << "End ts:" << (uint64_t)obj.end.time_since_epoch().count() << std::endl;
    os << "Sample len:" << (uint64_t)obj.sample_len / 1000000 << "ms" << std::endl;
    return os;
}


cMessageBroker::cMessageBroker(cSetup *setup){
    if (setup) {
        this->setup = setup;
    }else{
        //todo KILL execution, we can not continue without setup module;
        this->setup = nullptr;
    }

    bytes_cnt_tx = 0;
    bytes_cnt_rx = 0;
    pkt_cnt_rx = 0;
    pkt_cnt_tx = 0;
    dup_cnt = 0;

    this->rtt_avg = 0;
    this->rtt_min = 9999999;
    this->rtt_max = 0;
    this->pkt_rcvd = 0;
    this->pkt_sent = 0;
    this->ooo_cnt = 0;

    for (int i = 0; i < 2; i++){
        this->sampled_int[i].first = true;
        this->sampled_int[i].first_seq = 0;
        this->sampled_int[i].pkt_cnt = 0;
        this->sampled_int[i].ts_limit = 0;
        this->sampled_int[i].seq = 0;
        this->sampled_int[i].ooo = 0;
        this->sampled_int[i].dup = 0;
        this->sampled_int[i].rtt_sum = 0;
        this->sampled_int[i].delay_sum = 0;
        this->sampled_int[i].bytes = 0;
        this->sampled_int[i].jitter_sum = 0;
        this->sampled_int[i].last_seen_seq = 0;
    }
    json_first = true;
    timer_slot_interval = setup->getSampleLen();
}



cMessageBroker::~cMessageBroker(){
    if (setup->isServer()){
        //std::cout << this->pkt_cnt_rx << "/" << this->pkt_cnt_tx << std::endl;
    }
}

void cMessageBroker::push(gen_msg_t *msg, conn_t *conn){
    struct t_msg_t *t_msg;
    t_msg = new t_msg_t;
    t_msg->tp = std::chrono::system_clock::now();
    t_msg->msg = msg;
    t_msg->conn = conn;
    msg_buf.push(t_msg);
}

void cMessageBroker::push_rx(gen_msg_t *msg){
    struct t_msg_t *t_msg;
    t_msg = new t_msg_t;
    t_msg->tp = std::chrono::system_clock::now();
    t_msg->msg = msg;
    msg_buf_rx.push(t_msg);
}

void cMessageBroker::push_tx(gen_msg_t *msg){
    struct t_msg_t *t_msg;
    t_msg = new t_msg_t;
    t_msg->tp = std::chrono::system_clock::now();
    t_msg->msg = msg;
    msg_buf_tx.push(t_msg);
}

void cMessageBroker::push_lp(gen_msg_t *msg){
    struct t_msg_t *t_msg;
    t_msg = new t_msg_t;
    t_msg->tp = std::chrono::system_clock::now();
    t_msg->msg = msg;
    msg_buf_lp.push(t_msg);
}

void cMessageBroker::push_hp(gen_msg_t *msg){
    struct t_msg_t *t_msg;
    t_msg = new t_msg_t;
    t_msg->tp = std::chrono::system_clock::now();
    t_msg->msg = msg;
    msg_buf_hp.push(t_msg);
}

void cMessageBroker::push_hp(gen_msg_t *msg, conn_t * conn){
    struct t_msg_t *t_msg;
    t_msg = new t_msg_t;
    t_msg->tp = std::chrono::system_clock::now();
    t_msg->msg = msg;
    t_msg->conn = conn;
    msg_buf_hp.push(t_msg);
}


void cMessageBroker::run(){
    uint64_t rx_ts;
    uint64_t tx_ts;
    t_msg_t * rx_msg;
    t_msg_t * tx_msg;
    uint64_t first=0;
    if (setup->isClient()){
        while (!setup->isDone()){
            usleep(1);
            //HI priority message queue
            while (msg_buf_hp.front()){
                processAndDeleteClientMessage(*msg_buf_hp.front());
                delete *msg_buf_hp.front();
                msg_buf_hp.pop();
            }
            while ((msg_buf_rx.front())||(msg_buf_tx.front())){
                rx_ts = 0;
                tx_ts = 0;
                if (msg_buf_rx.front()){
                    rx_msg = *msg_buf_rx.front();
                    rx_ts= rx_msg->tp.time_since_epoch().count();
                }
                if (msg_buf_tx.front()){
                    tx_msg = *msg_buf_tx.front();
                    tx_ts= tx_msg->tp.time_since_epoch().count();
                }
                if (not rx_ts && not tx_ts ) continue;

                if ((rx_ts < tx_ts) || ((tx_ts)&&(rx_ts == 0))){
                    if (first==0) first = tx_ts;
                    processAndDeleteClientMessage(*msg_buf_tx.front());
                    delete *msg_buf_tx.front();
                    msg_buf_tx.pop();
                }
                if ((rx_ts >= tx_ts) || ((tx_ts == 0)&&(rx_ts))){
                    processAndDeleteClientMessage(*msg_buf_rx.front());
                    delete *msg_buf_rx.front();
                    msg_buf_rx.pop();
                }
            }
            // LOW priority message queue
            while (msg_buf_lp.front()){
                processAndDeleteClientMessage(*msg_buf_lp.front());
                delete *msg_buf_lp.front();
                msg_buf_lp.pop();
            }
        }
        //Todo: better signaling needed (do not shutdown mBroker until everything is done)
        while (msg_buf_lp.front()){
            processAndDeleteClientMessage(*msg_buf_lp.front());
            delete *msg_buf_lp.front();
            msg_buf_lp.pop();
        }
    }
    if (setup->isServer()){
        while (!setup->isDone()){
            usleep(1);
            //HI priority message queue
            while (msg_buf_hp.front()){
                processAndDeleteServerMessage(*msg_buf_hp.front());
                delete *msg_buf_hp.front();
                msg_buf_hp.pop();
            }
            //MAIN queue
            while (msg_buf.front()){
                processAndDeleteServerMessage(*msg_buf.front());
                delete *msg_buf.front();
                msg_buf.pop();
            }
            //LOW priority message queue
            while (msg_buf_lp.front()){
                processAndDeleteServerMessage(*msg_buf_lp.front());
                delete *msg_buf_lp.front();
                msg_buf_lp.pop();
            }
        }
    }
}

void cMessageBroker::processAndDeleteServerMessage(t_msg_t *tMsg) {
    std::chrono::system_clock::time_point tp = tMsg->tp;
    gen_msg_t *msg = tMsg->msg;
    uint64_t ts;
    switch(msg->type){
        case MSG_RX_PKT:
            tMsg->conn->pkt_rx_cnt++;
            tMsg->conn->bytes_rx_cnt+=tMsg->conn->size;
            tMsg->conn->last_pkt_rcvd = tp;
            if (not setup->silent()) {
                if (tMsg->conn->J_par){
                    *tMsg->conn->output << prepServerDataRec(tMsg, RX);
                    break;
                }
            }
            break;
        case MSG_TX_PKT:
            tMsg->conn->pkt_tx_cnt++;
            tMsg->conn->bytes_tx_cnt+=tMsg->conn->size;
            if (not setup->silent()) {
                if (tMsg->conn->J_par) {
                    *tMsg->conn->output << prepServerDataRec(tMsg, TX);
                    break;
                }
            }
            break;
        case MSG_KEEP_ALIVE:
            //std::cout << "MSG_KEEP_ALIVE" << std::endl;
            break;
        case MSG_SOCK_TIMEOUT:
            //std::cout << "MSG_SOCK_TIMEOUT" << std::endl;
            break;
        case MSG_TIMER_ONE:
            //std::cout << "MSG_TIMER_ONE" << std::endl;
            break;
        case MSG_OUTPUT_INIT:
            //std::cout << "MSG_OUTPUT_INIT" << std::endl;
            tMsg->conn->start = tp;
            tMsg->conn->end = tMsg->conn->start;
            if (tMsg->conn->fout.is_open()){
                tMsg->conn->output = &tMsg->conn->fout;
                if (setup->self_check() == SETUP_CHCK_VER) *tMsg->conn->output << setup->get_version().c_str();
            }else{
                tMsg->conn->output = &std::cout;
            }
            *tMsg->conn->output << prepServerHeader(tMsg->conn);
            break;

        case MSG_SLOT_TIMER_START:
            break;

        case MSG_SLOT_TIMER_STOP:
            break;
        case MSG_SLOT_TIMER_TICK:
            if (tMsg->conn->J_par && tMsg->conn->fout.is_open()){
                ts = (tp.time_since_epoch().count() * ((chrono::system_clock::period::num * uint64_t(1000000000L)) / chrono::system_clock::period::den));
                for (int direction = 0; direction < 2; direction ++){
                    *tMsg->conn->output << closeServerDataRecSlot(ts, tMsg, direction);
                }
            }
            break;

        case MSG_TIMER_END:
            //Not used in server
            //std::cout << "MSG_TIMER_END" << std::endl;
            //tMsg->conn->end = tp;
            break;
        case MSG_OUTPUT_CLOSE:
            //std::cout << "MSG_OUTPUT_CLOSE" << std::endl;
            //*output << c_stats->getReport();
            ts = (tp.time_since_epoch().count() * ((chrono::system_clock::period::num * uint64_t(1000000000L)) / chrono::system_clock::period::den));
            tMsg->conn->end = tp;
            //Flush buffered data
            if (tMsg->conn->J_par && tMsg->conn->fout.is_open()) {
                if (tMsg->conn->sample_len){
                    *tMsg->conn->output << prepServerFinalDataRec(ts, tMsg, TX);
                    *tMsg->conn->output << prepServerFinalDataRec(ts, tMsg, RX);
                }
            }
            //std::cout << "conn info:: " << *tMsg->conn << std::endl;
            if (tMsg->conn->J_par && (tMsg->conn->fout.is_open()||(tMsg->conn->output == &std::cout))) {
                *tMsg->conn->output << "\n\t],";
                *tMsg->conn->output << "\n\t\"server_stats\": {";
                *tMsg->conn->output << "\n\t\t\"tx_pkts\" :"<< tMsg->conn->pkt_tx_cnt << ",";
                *tMsg->conn->output << "\n\t\t\"rx_pkts\" :"<< tMsg->conn->pkt_rx_cnt << ",";
                *tMsg->conn->output << "\n\t\t\"tx_bytes\" :"<< tMsg->conn->bytes_tx_cnt << ",";
                *tMsg->conn->output << "\n\t\t\"rx_bytes\" :"<< tMsg->conn->bytes_rx_cnt << ",";
                *tMsg->conn->output << "\n\t\t\"ooo_pkts\" :"<< tMsg->conn->ooo_cnt << ",";
                *tMsg->conn->output << "\n\t\t\"dup_pkts\" :"<< tMsg->conn->dup_cnt << ",";
                *tMsg->conn->output << "\n\t\t\"duration\" :"<< (std::chrono::duration_cast<std::chrono::milliseconds>(tMsg->conn->end - tMsg->conn->start).count())/1000.0;
                *tMsg->conn->output << "\n\t}";
                *tMsg->conn->output << "\n}"<<std::endl;
            }else{

            }
            if (fout.is_open()){
                fout.close();
                output = &std::cout;
            }
            tMsg->conn->finished = true;
            break;
        default:
            std::cerr << "ERROR :: UNKNOWN MSG TYPE RECEIVED! " << (uint16_t) msg->type << std::endl;
    }
    delete msg;
    msg = nullptr;
}

void cMessageBroker::processAndDeleteClientMessage(t_msg_t *tMsg){
    std::chrono::system_clock::time_point tp = tMsg->tp;
    gen_msg_t *msg = tMsg->msg;
    uint64_t ts;
    uint64_t pkt_server_ts;
    //std::cout << "MSG_RECEIVED:" << (::uint16_t)msg->type << std::endl;
    switch(msg->type){
        //std::cout << "MSG_CNT_RECEIVED:" << (::uint16_t)ping_msg->code << std::endl;
        case MSG_RX_CNT:
            //Todo modify structure !!!! packet data not present - ONLY header was copied
            ping_msg = (struct ping_msg_t*) (msg);
            //std::cout << "MSG_RX_CNT:" << *ping_msg << std::endl;
            if (ping_msg->code == CNT_DONE_OK) {
                gen_msg_t * t = new gen_msg_t;
                t->type = MSG_OUTPUT_CLOSE;
                this->push_lp(t);
                usleep(500); //to be sure
                setup->setDone(true);
            }
            if (ping_msg->code == CNT_FNAME_OK) setup->setStarted(true);
            if (ping_msg->code == CNT_OUTPUT_REDIR) setup->setStarted(true);
            if (ping_msg->code == CNT_TERM){
                std::cout << "Connection terminated by server" << std::endl;
                setup->setStop(true);
            }
            break;

            //Probably not accessible
        case MSG_TX_CNT:
            //std::cout << "MSG_TX_CNT" << std::endl;
            break;

        case MSG_RX_PKT:
            //Todo process stats
            //std::cout << "MSG_RX_PKT" << std::endl;
            //Todo modify structure !!!! packet data not present - ONLY header was copied
            ping_pkt = (struct ping_pkt_t*) (msg);
            if (ping_pkt->size < HEADER_LENGTH) std::cerr << "Invalid RX Packet Size: " << ping_pkt->size << std::endl;
            ts = (tp.time_since_epoch().count() * ((chrono::system_clock::period::num * uint64_t(1000000000L)) / chrono::system_clock::period::den));
            pkt_rtt = ((tp.time_since_epoch().count() * ((chrono::system_clock::period::num * uint64_t(1000000000L)) / chrono::system_clock::period::den)) - (ping_pkt->sec * uint64_t(1000000000L)) - (ping_pkt->nsec))/1000000.0; //ms
            pkt_server_ts =  ping_pkt->server_sec * uint64_t(1000000000L) + ping_pkt->server_nsec;
            if (ts > pkt_server_ts){
                pkt_delay = (double)(ts - pkt_server_ts) / 1000000.0; //in ms
            }else{
                //leave it split!
                pkt_delay = (double)(pkt_server_ts - ts) / 1000000.0; //in ms
                pkt_delay = - pkt_delay;
            }

            if (setup->toJSON()) {
                *output << prepDataRec(ts, pkt_server_ts, RX, ping_pkt->size, ping_pkt->seq, pkt_rtt, pkt_delay);
                break;
            }
            *output << pingOutputRec(ts, RX, ping_pkt->size, ping_pkt->seq, pkt_rtt);
            //c_stats->pktRecv(ts, ping_pkt->size, ping_pkt->seq, pkt_rtt);
            break;

        case MSG_TX_PKT:
            ping_pkt = (struct ping_pkt_t*) (msg);
            if (ping_pkt->size < HEADER_LENGTH) std::cerr << "Invalid TX Packet Size: " << ping_pkt->size << std::endl;
            ts = (tp.time_since_epoch().count() * ((chrono::system_clock::period::num * uint64_t(1000000000L)) / chrono::system_clock::period::den));
            if (setup->toJSON()){
                *output << prepDataRec(ts, 0, TX, ping_pkt->size, ping_pkt->seq, 0, 0);
                break;
            }else{
                pkt_cnt_tx ++;
                break;
            }
            //c_stats->pktSent(ts, ping_pkt->size, ping_pkt->seq);
            break;
        case MSG_KEEP_ALIVE:
            //std::cout << "MSG_KEEP_ALIVE" << std::endl;
            break;
        case MSG_SOCK_TIMEOUT:
            //std::cout << "MSG_SOCK_TIMEOUT" << std::endl;
            break;

        case MSG_SLOT_TIMER_START:
            //std::cout << "MSG_SLOT_TIMER_START" << std::endl;
            break;

        case MSG_SLOT_TIMER_STOP:
            //std::cout << "MSG_SLOT_TIMER_STOP" << std::endl;
            break;

        case MSG_SLOT_TIMER_TICK:
            ts = (tp.time_since_epoch().count() * ((chrono::system_clock::period::num * uint64_t(1000000000L)) / chrono::system_clock::period::den));
            for (int direction = 0; direction < 2; direction ++){
                *output << closeDataRecSlot(ts, direction);
            }
            break;

        case MSG_TIMER_ONE:
            //
            break;

        case MSG_OUTPUT_INIT:
            //std::cout << "MSG_OUTPUT_INIT" << std::endl;
            start = tp;
            end = start;
            if (setup->getFilename().length() && setup->outToFile()) {
                fout.open(setup->getFilename().c_str());
                if (fout.is_open()){
                    output = &fout;
                    if (setup->self_check() == SETUP_CHCK_VER) *output << setup->get_version().c_str();
                }else{
                    output = setup->getOutput();
                }
            } else {
                output = setup->getOutput();
            }
            *output << prepHeader();
            break;

        case MSG_TIMER_END:
            end = tp;
            break;
        case MSG_OUTPUT_CLOSE:
            //std::cout << "MSG_OUTPUT_CLOSE" << std::endl;
            //*output << c_stats->getReport();
            ts = (tp.time_since_epoch().count() * ((chrono::system_clock::period::num * uint64_t(1000000000L)) / chrono::system_clock::period::den));
            //Flush buffered data
            if (not setup->silent()) {
                if (setup->getSampleLen()){
                    *output << prepFinalDataRec(ts, TX);
                    *output << prepFinalDataRec(ts, RX);
                }
            }
            if (setup->toJSON()) {
                *output << "\n\t],";
                *output << "\n\t\"client_stats\": {";
                *output << "\n\t\t\"tx_pkts\" :"<< pkt_cnt_tx << ",";
                *output << "\n\t\t\"rx_pkts\" :"<< pkt_cnt_rx << ",";
                *output << "\n\t\t\"tx_bytes\" :"<< bytes_cnt_tx << ",";
                *output << "\n\t\t\"rx_bytes\" :"<< bytes_cnt_rx << ",";
                *output << "\n\t\t\"ooo_pkts\" :"<< ooo_cnt << ",";
                *output << "\n\t\t\"dup_pkts\" :"<< dup_cnt << ",";
                *output << "\n\t\t\"duration\" :"<< (std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count())/1000.0;
                *output << "\n\t}";
                *output << "\n}"<<std::endl;
            }else{
                rtt_avg = (float)(rtt_sum / pkt_cnt_rx);
                *output << std::endl;
                *output << ".::. " << pkt_cnt_tx << " packets transmitted, " << pkt_cnt_rx << " packets received, ";
                if (pkt_cnt_tx){
                    *output << std::setprecision(3) << std::fixed << 100 * (double)(1.0-((double)pkt_cnt_rx / (double)pkt_cnt_tx)) << "% packet loss" ;
                }else{
                    *output << "100% packet loss" ;
                }
                *output << std::endl;
                *output << ".::. round-trip time min/avg/max = "<< std::setprecision(3) << std::fixed << rtt_min << "/" << rtt_avg << "/" << rtt_max << " ms";
                *output << ", test duration " << (std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count())/1000.0 << " s";
                *output << std::endl;
            }
            if (fout.is_open()){
                fout.close();
                output = &std::cout;
            }
            break;
        default:
            std::cerr << "ERROR :: UNKNOWN MSG TYPE RECEIVED!";
    }
    //std::cout << "MSG delete at: " << msg << std::endl;
    delete msg;
    msg = nullptr;
};



std::string cMessageBroker::prepHeader() {
    stringstream header;
    if (setup->toJSON()){
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        header << "{\n\t\"info\": {";
        header << "\n\t\t\"start\":\"" << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << "\",";
        header << "\n\t\t\"version\":\"" << setup->getVersion() << "\",";
        if (setup->getAddrFamily() == AF_INET) header << "\n\t\t\"ip_family\":\"IPv4\",";
        if (setup->getAddrFamily() == AF_INET6) header << "\n\t\t\"ip_family\":\"IPv6\",";
        header << "\n\t\t\"remote\":\"" << setup->getHostname().c_str() << "\",";
        header << "\n\t\t\"slot_duration\":" << setup->getSampleLen()/1000000000.0 << ",";
        header << "\n\t\t\"flow_id\":" << setup->getFlowID();
        header << "\n\t},";
        header << "\n\t\"client_data\": [";
        return header.str();
    }
    if (setup->toCSV()){
        header << "C_TimeStamp;C_Direction;C_PacketSize;C_From;C_Sequence;C_RTT;C_Delta;C_RX_Rate;C_To;C_TX_Rate;\n";
        return header.str();
    }
    header << ".::. Pinging " << setup->getHostname() << " with " << setup->getPacketSize() << " bytes of data:" << endl;
    return header.str();
}

std::string cMessageBroker::prepServerHeader(conn_t * conn) {
    stringstream header;
    if (conn->J_par && (conn->fout.is_open()||(conn->output == &std::cout))){
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        header << "{\n\t\"info\": {";
        header << "\n\t\t\"start\":\"" << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << "\",";
        header << "\n\t\t\"version\":\"" << setup->getVersion() << "\",";
        if (conn->family == AF_INET) header << "\n\t\t\"socket_ip_family\":\"IPv4\",";
        if (conn->family == AF_INET6) header << "\n\t\t\"socket_ip_family\":\"IPv6 & IPv4\",";
        header << "\n\t\t\"remote\":\"" << conn->client_ip << "\",";
        header << "\n\t\t\"slot_duration\":" << conn->sample_len / 1000000000L << ",";
        header << "\n\t\t\"flow_id\":" << conn->conn_id;
        header << "\n\t},";
        header << "\n\t\"server_data\": [";
        return header.str();
    }
    if (setup->toCSV()){
        header << "C_TimeStamp;C_Direction;C_PacketSize;C_From;C_Sequence;C_RTT;C_Delta;C_RX_Rate;C_To;C_TX_Rate;\n";
        return header.str();
    }
    //header << ".::. Pinging " << setup->getHostname() << " with " << setup->getPacketSize() << " bytes of data:" << endl;
    return header.str();
}

std::string cMessageBroker::closeServerDataRecSlot(const uint64_t ts,t_msg_t * tMsg, const uint8_t dir){
    stringstream ss;
    conn_t * conn = tMsg->conn;
    conn->sampled_int[dir].seq++;
    conn->sampled_int[dir].ts_limit += conn->sample_len;
    //for discusion: base it on server o client (silence) setup?
    if (!setup->silent()){
        if (conn->sampled_int[dir].first == false){
            ss << ",\n\t\t{";
        }else{
            ss << "\n\t\t{";
            conn->sampled_int[TX].first = false;
            conn->sampled_int[RX].first = false;
        }
        if (conn->sampled_int[dir].pkt_cnt){
            ss << "\n\t\t\t\"ts\":"  << std::setprecision(6) << std::fixed << (double)(ts/1000000000.0) << ",";
            if (dir == TX){
                ss << "\n\t\t\t\"dir\":\"tx\",";
            }
            if (dir == RX){
                ss << std::setprecision(6);
                ss << "\n\t\t\t\"dir\":\"rx\",";
                ss << "\n\t\t\t\"loss\":" << min(1.0, 1.0 - (float)conn->sampled_int[dir].pkt_cnt / (float)((conn->sampled_int[dir].last_seen_seq)-conn->sampled_int[dir].first_seq)) << ","; //in ms
                ss << "\n\t\t\t\"delay\":" << std::setprecision(6) << std::fixed << (double)(conn->sampled_int[dir].delay_sum/conn->sampled_int[dir].pkt_cnt/1000000.0) << ","; //in ms
                ss << "\n\t\t\t\"jitter\":" << std::setprecision(6) << std::fixed << (double)(conn->sampled_int[dir].jitter_sum/conn->sampled_int[dir].pkt_cnt/1000000.0) << ","; //in ms
                ss << "\n\t\t\t\"ooo_pkts\":" << conn->sampled_int[dir].ooo << ",";
                ss << "\n\t\t\t\"dup_pkts\":" << conn->sampled_int[dir].dup << ",";
            }
            ss << "\n\t\t\t\"pkts\":" << conn->sampled_int[dir].pkt_cnt << ",";
            ss << "\n\t\t\t\"bytes\":" << conn->sampled_int[dir].bytes << ",\n\t\t\t\"seq\":" << conn->sampled_int[dir].seq << "\n\t\t}";
        }else{
            ss << "\n\t\t\t\"ts\":"  << std::setprecision(6) << std::fixed << (double)(ts/1000000000.0) << ",";
            if (dir == TX){
                ss << "\n\t\t\t\"dir\":\"tx\",";
            }
            if (dir == RX){
                ss << "\n\t\t\t\"dir\":\"rx\",";
                ss << std::setprecision(6) << std::fixed;
                if (conn->sampled_int[TX].last_seen_seq > conn->sampled_int[RX].last_seen_seq){
                    ss << "\n\t\t\t\"loss\":" << 1.0 << ",";
                }
                else{
                    ss << "\n\t\t\t\"loss\":" << 0.0 << ",";
                }
                ss << "\n\t\t\t\"ooo_pkts\":" << 0 << ",";
                ss << "\n\t\t\t\"dup_pkts\":" << 0 << ",";
                ss << "\n\t\t\t\"delay\":" << 0 << ","; //in ms
                ss << "\n\t\t\t\"jitter\":" << 0 << ","; //in ms
            }
            ss << "\n\t\t\t\"pkts\":" << 0 << ",";
            ss << "\n\t\t\t\"bytes\":" << 0 << ",\n\t\t\t\"seq\":" << conn->sampled_int[dir].seq << "\n\t\t}";
        }
    }
    if (dir == RX) {
        conn->sampled_int[dir].ooo = 0;
        conn->sampled_int[dir].dup = 0;
        conn->sampled_int[dir].pkt_cnt = 0;
        conn->sampled_int[dir].bytes = 0;
        conn->sampled_int[dir].first_seq = conn->sampled_int[dir].last_seen_seq;
        conn->sampled_int[dir].rtt_sum = 0;
        conn->sampled_int[dir].delay_sum = 0;
        conn->sampled_int[dir].jitter_sum  = 0;
    }else{
        //do nothing
    }
    conn->sampled_int[dir].bytes = 0;
    conn->sampled_int[dir].pkt_cnt = 0;
    return ss.str();
}


std::string cMessageBroker::closeDataRecSlot(const uint64_t ts, const uint8_t dir){
    stringstream ss;
    sampled_int[dir].seq++;
    sampled_int[dir].ts_limit += setup->getSampleLen();
    if (sampled_int[dir].first == false){
        ss << ",\n\t\t{";
    }else{
        ss << "\n\t\t{";
        sampled_int[TX].first = false;
        sampled_int[RX].first = false;
    }
    if (sampled_int[dir].pkt_cnt){
        ss << "\n\t\t\t\"ts\":"  << std::setprecision(6) << std::fixed << (double)(ts/1000000000.0) << ",";
        if (dir == TX){
            ss << "\n\t\t\t\"dir\":\"tx\",";
        }
        if (dir == RX){
            ss << std::setprecision(6);
            ss << "\n\t\t\t\"dir\":\"rx\",";
            ss << "\n\t\t\t\"loss\":" << min(1.0, 1.0 - (float)sampled_int[dir].pkt_cnt / (float)((sampled_int[dir].last_seen_seq)-sampled_int[dir].first_seq)) << ","; //in ms
            ss << "\n\t\t\t\"rtt\":" << sampled_int[dir].rtt_sum/sampled_int[dir].pkt_cnt << ","; //in ms
            ss << "\n\t\t\t\"delay\":" << sampled_int[dir].delay_sum/sampled_int[dir].pkt_cnt << ","; //in ms
            ss << "\n\t\t\t\"jitter\":" << sampled_int[dir].jitter_sum/sampled_int[dir].pkt_cnt << ","; //in ms
            ss << "\n\t\t\t\"ooo_pkts\":" << sampled_int[dir].ooo << ",";
            ss << "\n\t\t\t\"dup_pkts\":" << sampled_int[dir].dup << ",";
        }
        ss << "\n\t\t\t\"pkts\":" << sampled_int[dir].pkt_cnt << ",";
        ss << "\n\t\t\t\"bytes\":" << sampled_int[dir].bytes << ",\n\t\t\t\"seq\":" << sampled_int[dir].seq << "\n\t\t}";
    }else{
        ss << "\n\t\t\t\"ts\":"  << std::setprecision(6) << std::fixed << (double)(ts/1000000000.0) << ",";
        if (dir == TX){
            ss << "\n\t\t\t\"dir\":\"tx\",";
        }
        if (dir == RX){
            ss << "\n\t\t\t\"dir\":\"rx\",";
            ss << std::setprecision(6) << std::fixed;
            if (sampled_int[TX].last_seen_seq > sampled_int[RX].last_seen_seq){
                ss << "\n\t\t\t\"loss\":" << 1.0 << ",";
            }
            else{
                ss << "\n\t\t\t\"loss\":" << 0.0 << ",";
            }
            ss << "\n\t\t\t\"ooo_pkts\":" << 0 << ",";
            ss << "\n\t\t\t\"dup_pkts\":" << 0 << ",";
            ss << "\n\t\t\t\"rtt\":" << 0 << ","; //in ms
            ss << "\n\t\t\t\"jitter\":" << 0 << ","; //in ms
        }
        ss << "\n\t\t\t\"pkts\":" << 0 << ",";
        ss << "\n\t\t\t\"bytes\":" << 0 << ",\n\t\t\t\"seq\":" << sampled_int[dir].seq << "\n\t\t}";
    }
    if (dir == RX) {
        sampled_int[dir].ooo = 0;
        sampled_int[dir].dup = 0;
        sampled_int[dir].pkt_cnt = 0;
        sampled_int[dir].bytes = 0;
        sampled_int[dir].first_seq = sampled_int[dir].last_seen_seq;
        sampled_int[dir].rtt_sum = 0;
        sampled_int[dir].delay_sum = 0;
        sampled_int[dir].jitter_sum  = 0;
    }else{
        //do nothing
    }
    sampled_int[dir].bytes = 0;
    sampled_int[dir].pkt_cnt = 0;
    //FIXME
    //sampled_int[dir].first_seq = sampled_int[dir].last_seen_seq;
    if (setup->silent()) return "";
    return ss.str();
}


std::string cMessageBroker::prepServerDataRec(t_msg_t* tMsg, const uint8_t dir){
    conn_t * conn = tMsg->conn;
    ping_pkt_t * ping_pkt = (struct ping_pkt_t*) (tMsg->msg);
    if (conn->size < HEADER_LENGTH) std::cerr << "Invalid RX Packet Size: " << conn->size << std::endl;
    const uint64_t ts = ping_pkt->server_sec * uint64_t(1000000000L) + ping_pkt->server_nsec;
    const uint64_t pkt_client_ts =  ping_pkt->sec * uint64_t(1000000000L) + ping_pkt->nsec;
    const uint16_t size = conn->size;
    const uint64_t seq = ping_pkt->seq;
    //const uint64_t delay = 0;

    if (ts > pkt_client_ts){
        pkt_delay = (double)(ts - pkt_client_ts) / 1000000.0; //in ms
    }else{
        //leave it split!
        pkt_delay = (double)(pkt_client_ts - ts) / 1000000.0; //in ms
        pkt_delay = - pkt_delay;
    }
    //std::cout << ts << " / " << pkt_client_ts<< "/" << pkt_delay << "/" << pkt_client_ts - ts << std::endl;
    stringstream ss;
    if (conn->sample_len){
        // if (ts < sampled_int[dir].ts_limit){
        if (dir == RX){
            if (conn->sampled_int[dir].last_seen_seq == seq){
                conn->dup_cnt++;
                conn->sampled_int[dir].dup++;
                return ss.str();
            }
            if (conn->sampled_int[dir].last_seen_seq > seq){
                conn->ooo_cnt++;
                conn->sampled_int[dir].ooo++;
                return ss.str();
            }
            //conn->pkt_rx_cnt++;
            //conn->bytes_rx_cnt += size;
            conn->sampled_int[dir].delay_sum += pkt_delay;
            //J(i) = J(i-1) + (|D(i-1,i)| - J(i-1))/16   viz RFC3550 (RTP/RTCP)
            conn->jt_diff = (pkt_delay) - conn->jt_delay_prev;
            conn->jt_delay_prev = pkt_delay;
            if (conn->jt_diff < 0) conn->jt_diff = -conn->jt_diff;
            conn->jt_prev = conn->jitter;
            conn->jitter = conn->jt_prev + (1.0 / 16.0) * (conn->jt_diff - conn->jt_prev);
            //jitter_sum += jitter;
            conn->sampled_int[dir].jitter_sum += conn->jitter;
        } else{
            //conn->pkt_tx_cnt++;
            //conn->bytes_tx_cnt += conn->ret_size;
        }
        conn->sampled_int[dir].pkt_cnt++;
        conn->sampled_int[dir].bytes += size;
        conn->sampled_int[dir].last_seen_seq = seq;
        return ss.str();
        //}else{}
    }else{
        if (json_first){
            json_first = false;
            ss << "\n\t\t{";
        }else{
            ss << ",\n\t\t{";
        }
        ss << "\n\t\t\t\"ts\":"  << std::setprecision(6) << std::fixed << (double)(ts/1000000000.0) << ",";
        if (dir == TX){
            //pkt_cnt_tx++;
            //conn->bytes_tx_cnt += conn->ret_size;
            ss << "\n\t\t\t\"dir\":\"tx\",";
            ss << "\n\t\t\t\"size\":" << conn->ret_size << ",\n\t\t\t\"seq\":" << seq << "\n\t\t}";
        }
        if (dir == RX){
            if (conn->sampled_int[dir].last_seen_seq == seq){
                conn->dup_cnt++;
                return ss.str();
            }
            if (conn->sampled_int[dir].last_seen_seq > seq){
                conn->ooo_cnt++;
                return ss.str();
            }
            ss << "\n\t\t\t\"cts\":"  << std::setprecision(6) << std::fixed << (double)(pkt_client_ts/1000000000.0) << ",";
            //pkt_cnt_rx++;
            //conn->bytes_rx_cnt += size;
            ss << "\n\t\t\t\"dir\":\"rx\",";
            ss << "\n\t\t\t\"delay\":"  << std::setprecision(6) << std::fixed << (pkt_delay) << ",";
            //J(i) = J(i-1) + (|D(i-1,i)| - J(i-1))/16   viz RFC3550 (RTP/RTCP)
            jt_diff = pkt_delay - jt_rtt_prev;
            jt_rtt_prev = pkt_delay;
            if (jt_diff < 0) jt_diff = -jt_diff;
            jt_prev = jitter;
            jitter = jt_prev + (1.0 / 16.0) * (jt_diff - jt_prev);
            //jitter_sum += jitter;
            ss << "\n\t\t\t\"jitter\":"  << std::setprecision(6) << std::fixed << jitter << ",";
            ss << "\n\t\t\t\"size\":" << size << ",\n\t\t\t\"seq\":" << seq << "\n\t\t}";
        }
        sampled_int[dir].last_seen_seq = seq;
        return ss.str();
    }
    return "Initial string";
}




// double pkt_rtt, sample_cum_rtt;
// uint64_t sample_seq_first, sample_pkt_cnt;
// uint64_t sample_ts_limit;

std::string cMessageBroker::prepDataRec(const uint64_t ts, const uint64_t pkt_server_ts, const uint8_t dir, const uint16_t size, const uint64_t seq, const float rtt, const float delay){
    stringstream ss;
    if (setup->getSampleLen()){
        // if (ts < sampled_int[dir].ts_limit){
        if (dir == RX){
            if (sampled_int[dir].last_seen_seq == seq){
                dup_cnt++;
                sampled_int[dir].dup++;
                return ss.str();
            }
            if (sampled_int[dir].last_seen_seq > seq){
                ooo_cnt++;
                sampled_int[dir].ooo++;
                return ss.str();
            }
            pkt_cnt_rx++;
            bytes_cnt_rx += size;
            sampled_int[dir].rtt_sum += rtt;
            sampled_int[dir].delay_sum += delay;
            //J(i) = J(i-1) + (|D(i-1,i)| - J(i-1))/16   viz RFC3550 (RTP/RTCP)
            jt_diff = rtt - jt_rtt_prev;
            jt_rtt_prev = rtt;
            if (jt_diff < 0) jt_diff = -jt_diff;
            jt_prev = jitter;
            jitter = jt_prev + (1.0 / 16.0) * (jt_diff - jt_prev);
            //jitter_sum += jitter;
            sampled_int[dir].jitter_sum += jitter;
        } else{
            pkt_cnt_tx++;
            bytes_cnt_tx += size;
        }
        sampled_int[dir].pkt_cnt++;
        sampled_int[dir].bytes += size;
        sampled_int[dir].last_seen_seq = seq;
        return ss.str();
        //}else{}
    } else{
        if (json_first){
            json_first = false;
            ss << "\n\t\t{";
        }else{
            ss << ",\n\t\t{";
        }
        ss << "\n\t\t\t\"ts\":"  << std::setprecision(6) << std::fixed << (double)(ts/1000000000.0) << ",";
        if (dir == TX){
            pkt_cnt_tx++;
            bytes_cnt_tx += size;
            ss << "\n\t\t\t\"dir\":\"tx\",";
        }
        if (dir == RX){
            if (sampled_int[dir].last_seen_seq == seq){
                dup_cnt++;
                //return ss.str();
            }
            if (sampled_int[dir].last_seen_seq > seq){
                ooo_cnt++;
                //return ss.str();
            }
            ss << "\n\t\t\t\"sts\":"  << std::setprecision(6) << std::fixed << (double)(pkt_server_ts/1000000000.0) << ",";
            pkt_cnt_rx++;
            bytes_cnt_rx += size;
            ss << "\n\t\t\t\"dir\":\"rx\",";
            ss << "\n\t\t\t\"rtt\":" << std::setprecision(6) << rtt << ","; //in ms
            ss << "\n\t\t\t\"delay\":" << std::setprecision(6) << delay << ","; //in ms
            //J(i) = J(i-1) + (|D(i-1,i)| - J(i-1))/16   viz RFC3550 (RTP/RTCP)
            jt_diff = rtt - jt_rtt_prev;
            jt_rtt_prev = rtt;
            if (jt_diff < 0) jt_diff = -jt_diff;
            jt_prev = jitter;
            jitter = jt_prev + (1.0 / 16.0) * (jt_diff - jt_prev);
            //jitter_sum += jitter;
            ss << "\n\t\t\t\"jitter\":" << std::setprecision(6) << jitter << ","; //in ms
        }
        ss << "\n\t\t\t\"size\":" << size << ",\n\t\t\t\"seq\":" << seq << "\n\t\t}";
        sampled_int[dir].last_seen_seq = seq;
        //return ss.str();
    }
    if (setup->silent()) return "";
    return ss.str();
}

std::string cMessageBroker::pingOutputRec(const uint64_t ts, const uint8_t dir, const uint16_t size, const uint64_t seq, const float rtt){
    stringstream ss;
    if (setup->showTimeStamps()){
        ss << std::setprecision(6) << std::fixed << (double)(ts/1000000000.0) << " ";
    }
    if (dir == TX){
        pkt_cnt_tx++;
        bytes_cnt_tx += size;
    }
    if (dir == RX){
        if (last_seen_seq == seq){
            dup_cnt++;
            ss << "DUP! " << "seq=" << seq << std::endl;
            return ss.str();
        }
        if (last_seen_seq > seq){
            ooo_cnt++;
            ss << "Out Of Order! " << "seq=" << seq << std::endl;
            return ss.str();
        }
        ss << size << " bytes from " << setup->getHostname() << " ";
        pkt_cnt_rx++;
        bytes_cnt_rx += size;
        rtt_sum += rtt;
        rtt_min = min(rtt_min, rtt);
        rtt_max = max(rtt_max, rtt);
        ss << "seq=" << seq << " ";
        ss << "time=" << std::setprecision(6) << rtt << " "; //in ms
        //J(i) = J(i-1) + (|D(i-1,i)| - J(i-1))/16   viz RFC3550 (RTP/RTCP)
        jt_diff = rtt - jt_rtt_prev;
        jt_rtt_prev = rtt;
        if (jt_diff < 0) jt_diff = -jt_diff;
        jt_prev = jitter;
        jitter = jt_prev + (1.0 / 16.0) * (jt_diff - jt_prev);
        //jitter_sum += jitter;
        ss << "jitter=" << std::setprecision(6) << jitter << ","; //in ms
    }
    last_seen_seq = seq;
    ss  << std::endl;
    if (setup->silent()) return "";
    return ss.str();
}


std::string cMessageBroker::prepFinalDataRec(const uint64_t ts, const uint8_t dir){
    stringstream ss;
    if (sampled_int[dir].pkt_cnt){
        sampled_int[dir].seq++;
        //sampled_int[dir].ts_limit += setup->getSampleLen();
        if (sampled_int[dir].first == false){
            ss << ",\n\t\t{";
        }else{
            sampled_int[TX].first = false;
            sampled_int[RX].first = false;
        }
        ss << "\n\t\t\t\"ts\":"  << std::setprecision(6) << std::fixed << (double)(ts/1000000000.0) << ",";
        if (dir == TX){
            ss << "\n\t\t\t\"dir\":\"tx\",";
        }
        if (dir == RX){
            ss << "\n\t\t\t\"dir\":\"rx\",";
            ss << std::setprecision(6);
            if ((sampled_int[dir].last_seen_seq-sampled_int[dir].first_seq) == 0){
                if (sampled_int[dir].last_seen_seq == sampled_int[TX].last_seen_seq){
                    ss << "\n\t\t\t\"loss\":0.0,";
                }else{
                    ss << "\n\t\t\t\"loss\":1.0,";
                }
            }else{
                ss << "\n\t\t\t\"loss\":" <<  min(1.0, 1.0 - (float)sampled_int[dir].pkt_cnt / (float)((sampled_int[dir].last_seen_seq)-sampled_int[dir].first_seq)) << ",";
            }
            ss << "\n\t\t\t\"rtt\":" << sampled_int[dir].rtt_sum/sampled_int[dir].pkt_cnt << ","; //in ms
            ss << "\n\t\t\t\"delay\":" << sampled_int[dir].delay_sum/sampled_int[dir].pkt_cnt << ","; //in ms
            ss << "\n\t\t\t\"jitter\":" << sampled_int[dir].jitter_sum/sampled_int[dir].pkt_cnt << ","; //in ms
            ss << "\n\t\t\t\"ooo_pkts\":" << sampled_int[dir].ooo << ",";
            ss << "\n\t\t\t\"dup_pkts\":" << sampled_int[dir].dup << ",";
        }
        ss << "\n\t\t\t\"pkts\":" << sampled_int[dir].pkt_cnt << ",";
        ss << "\n\t\t\t\"bytes\":" << sampled_int[dir].bytes << ",\n\t\t\t\"seq\":" << sampled_int[dir].seq << "\n\t\t}";
    }else{
        if (sampled_int[TX].pkt_cnt){
            sampled_int[dir].seq++;
            if (sampled_int[dir].first == false){
                ss << ",\n\t\t{";
            }else{
                sampled_int[TX].first = false;
                sampled_int[RX].first = false;
            }
            ss << "\n\t\t\t\"ts\":"  << std::setprecision(6)  << std::fixed << (double)(ts/1000000000.0) << ",";
            if (dir == TX){
                ss << "\n\t\t\t\"dir\":\"tx\",";
            }
            if (dir == RX){
                ss << std::setprecision(6);
                ss << "\n\t\t\t\"dir\":\"rx\",";
                ss << "\n\t\t\t\"loss\":" << 1.0 << ","; //in ms
                ss << "\n\t\t\t\"rtt\":"  << 0 << ","; //in ms
                ss << "\n\t\t\t\"delay\":"  << 0 << ","; //in ms
                ss << "\n\t\t\t\"jitter\":" << 0 << ","; //in ms
                ss << "\n\t\t\t\"ooo_pkts\":" << 0 << ",";
                ss << "\n\t\t\t\"dup_pkts\":" << 0 << ",";
            }
            ss << "\n\t\t\t\"pkts\":" << 0 << ",";
            ss << "\n\t\t\t\"bytes\":" << 0 << ",\n\t\t\t\"seq\":" << sampled_int[dir].seq << "\n\t\t}";
        }
    }
    if (setup->silent()) return "";
    return ss.str();
}

std::string cMessageBroker::prepServerFinalDataRec(const uint64_t ts, t_msg_t * tMsg, const uint8_t dir){
    stringstream ss;
    conn_t * conn = tMsg->conn;
    //std::cout << conn->conn_id << ": " << conn->bytes_rx_cnt << std::endl;
    if (conn->sampled_int[dir].pkt_cnt){
        conn->sampled_int[dir].seq++;
        //sampled_int[dir].ts_limit += setup->getSampleLen();
        if (conn->sampled_int[dir].first == false){
            ss << ",\n\t\t{";
        }else{
            conn->sampled_int[TX].first = false;
            conn->sampled_int[RX].first = false;
        }
        ss << "\n\t\t\t\"ts\":"  << std::setprecision(6) << std::fixed << (double)(ts/1000000000.0) << ",";
        if (dir == TX){
            ss << "\n\t\t\t\"dir\":\"tx\",";
        }
        if (dir == RX){
            ss << "\n\t\t\t\"dir\":\"rx\",";
            ss << std::setprecision(6);
            if ((conn->sampled_int[dir].last_seen_seq-conn->sampled_int[dir].first_seq) == 0){
                if (conn->sampled_int[dir].last_seen_seq == conn->sampled_int[TX].last_seen_seq){
                    ss << "\n\t\t\t\"loss\":0.0,";
                }else{
                    ss << "\n\t\t\t\"loss\":1.0,";
                }
            }else{
                ss << "\n\t\t\t\"loss\":" <<  min(1.0, 1.0 - (float)conn->sampled_int[dir].pkt_cnt / (float)((conn->sampled_int[dir].last_seen_seq)-conn->sampled_int[dir].first_seq)) << ",";
            }
            ss << "\n\t\t\t\"delay\":" << std::setprecision(6) << std::fixed << (double)(conn->sampled_int[dir].delay_sum/conn->sampled_int[dir].pkt_cnt/1000000.0) << ","; //in ms
            ss << "\n\t\t\t\"jitter\":" << std::setprecision(6) << std::fixed << (double)(conn->sampled_int[dir].jitter_sum/conn->sampled_int[dir].pkt_cnt/1000000.0) << ","; //in ms
            ss << "\n\t\t\t\"ooo_pkts\":" << conn->sampled_int[dir].ooo << ",";
            ss << "\n\t\t\t\"dup_pkts\":" << conn->sampled_int[dir].dup << ",";
        }
        ss << "\n\t\t\t\"pkts\":" << conn->sampled_int[dir].pkt_cnt << ",";
        ss << "\n\t\t\t\"bytes\":" << conn->sampled_int[dir].bytes << ",\n\t\t\t\"seq\":" << conn->sampled_int[dir].seq << "\n\t\t}";
    }else{
        if (conn->sampled_int[RX].pkt_cnt){
            conn->sampled_int[dir].seq++;
            if (conn->sampled_int[dir].first == false){
                ss << ",\n\t\t{";
            }else{
                conn->sampled_int[TX].first = false;
                conn->sampled_int[RX].first = false;
            }
            ss << "\n\t\t\t\"ts\":"  << std::setprecision(6)  << std::fixed << (double)(ts/1000000000.0) << ",";
            if (dir == TX){
                ss << "\n\t\t\t\"dir\":\"tx\",";
            }
            if (dir == RX){
                ss << std::setprecision(6);
                ss << "\n\t\t\t\"dir\":\"rx\",";
                ss << "\n\t\t\t\"loss\":" << 1.0 << ","; //in ms
                ss << "\n\t\t\t\"rtt\":"  << 0 << ","; //in ms
                ss << "\n\t\t\t\"jitter\":" << 0 << ","; //in ms
                ss << "\n\t\t\t\"ooo_pkts\":" << 0 << ",";
                ss << "\n\t\t\t\"dup_pkts\":" << 0 << ",";
            }
            ss << "\n\t\t\t\"pkts\":" << 0 << ",";
            ss << "\n\t\t\t\"bytes\":" << 0 << ",\n\t\t\t\"seq\":" << conn->sampled_int[dir].seq << "\n\t\t}";
        }
    }
    return ss.str();
}

/*

#ifndef _NOSTATS
//stats->pktReceived(conn_id, connection->curTv, rec_size, ping_pkt->seq, string(client_ip), saClient.sin_port);
//stats->pktSent(conn_id, connection->curTv, ret_size, ping_pkt->seq, string(client_ip), saClient.sin_port);
#endif
if (show || connection->fp != stdout) {

double delta = ((double) (connection->curTv.tv_sec - connection->refTv.tv_sec)*1000.0 + (double) (connection->curTv.tv_nsec - connection->refTv.tv_nsec) / 1000000.0);
ss.str("");
if (setup->toJSON(connection->J_par)) {
if (connection->pkt_cnt == 0){
ss << "{\n\t\"server_data\": [\n\t\t{";
}else{
ss << ",\n\t\t{";
}
}
if (setup->showTimeStamps(connection->D_par)) {
if (setup->toCSV(connection->C_par)) {
ss << connection->curTv.tv_sec << ".";
ss.fill('0');
ss.width(9);
ss << connection->curTv.tv_nsec;
ss << ";";
}
if (setup->toJSON(connection->J_par)) {
ss << "\"ts\":" << connection->curTv.tv_sec << ".";
ss.fill('0');
ss.width(9);
ss << connection->curTv.tv_nsec;
ss << ",\n\t\t\t";
}
if (!setup->toCSV(connection->C_par)&&!setup->toJSON(connection->J_par)) {
ss << "[" << connection->curTv.tv_sec << ".";
ss.fill('0');
ss.width(9);
ss << connection->curTv.tv_nsec;
ss << "] ";
}
} else {
if (setup->toCSV(connection->C_par)) {
ss << ";";
}
}
if (setup->wholeFrame(connection->H_par)) {
rec_size += 42;
ret_size += 42;
}

if (setup->toCSV(connection->C_par)) {
ss << rec_size << ";" << client_ip << ";" << ping_pkt->seq << ";xx;";
ss.setf(ios_base::right, ios_base::adjustfield);
ss.setf(ios_base::fixed, ios_base::floatfield);
ss.precision(3);
ss << delta << ";";
}
if (setup->toJSON(connection->J_par)) {
ss << "\"size\":" << rec_size << ",\n\t\t\t\"remote\":\"" << client_ip << "\",\n\t\t\t\"seq\":" << ping_pkt->seq << ",\n\t\t\t";
ss.setf(ios_base::right, ios_base::adjustfield);
ss.setf(ios_base::fixed, ios_base::floatfield);
ss.precision(3);
ss << "\"delta\":" << delta;
}
if (!setup->toCSV(connection->C_par)&&!setup->toJSON(connection->J_par)) {

ss << rec_size << " bytes from " << client_ip << ": req=" << ping_pkt->seq << " ttl=xx ";
ss.setf(ios_base::right, ios_base::adjustfield);
ss.setf(ios_base::fixed, ios_base::floatfield);
ss.precision(3);
ss << "delta=" << delta << " ms";
}
if (setup->showBitrate(connection->e_par)) {
if (setup->toCSV(connection->C_par)) {
ss.setf(ios_base::right, ios_base::adjustfield);
ss.setf(ios_base::fixed, ios_base::floatfield);
ss.precision(2);
ss << (1000 / delta) * rec_size * 8 / 1000 << ";";
}
if (setup->toJSON(connection->J_par)) {
ss.setf(ios_base::right, ios_base::adjustfield);
ss.setf(ios_base::fixed, ios_base::floatfield);
ss.precision(2);
ss << ",\n\t\t\t\"rx_bitrate\":" << (1000 / delta) * rec_size * 8 / 1000;
}
if (!setup->toCSV(connection->C_par)&&!setup->toJSON(connection->J_par)) {
ss.setf(ios_base::right, ios_base::adjustfield);
ss.setf(ios_base::fixed, ios_base::floatfield);
ss.precision(2);
ss << " rx_rate=" << (1000 / delta) * rec_size * 8 / 1000 << " kbps";
}
} else {
if (setup->toCSV(connection->C_par)) {
ss << ";";
}
}
if (setup->showSendBitrate(connection->E_par)) {
if (setup->toCSV(connection->C_par)) {
ss.setf(ios_base::right, ios_base::adjustfield);
ss.setf(ios_base::fixed, ios_base::floatfield);
ss.precision(2);
ss << (1000 / delta) * ret_size * 8 / 1000 << ";";
}
if (setup->toJSON(connection->J_par)) {
ss.setf(ios_base::right, ios_base::adjustfield);
ss.setf(ios_base::fixed, ios_base::floatfield);
ss.precision(2);
ss << ",\n\t\t\t\"tx_bitrate\":" << (1000 / delta) * ret_size * 8 / 1000;
}
if (!setup->toCSV(connection->C_par)&&!setup->toJSON(connection->J_par)) {
ss.setf(ios_base::right, ios_base::adjustfield);
ss.setf(ios_base::fixed, ios_base::floatfield);
ss.precision(2);
ss << " tx_rate=" << (1000 / delta) * ret_size * 8 / 1000 << " kbit/s";
}
ss << msg;
} else {
if (setup->toCSV(connection->C_par)) {
ss << ";";
}
}
if (setup->toJSON(connection->J_par)) {
ss << "\n\t\t}";
}else{
ss << endl;
}
if (setup->useTimedBuffer(connection->W_par)) {
connection->msg_store.push_back(ss.str());
} else {
fprintf(connection->fp, "%s", ss.str().c_str());

}
}

*/
