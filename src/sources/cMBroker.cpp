//
// Created by Ondřej Vondrouš on 2019-02-20.
//

#include "cMBroker.h"
#include "cSetup.h"
#include <chrono>
#include <iomanip>

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



cMessageBroker::cMessageBroker(cSetup *setup, cStats *stats){
    dup = 0;
    if (setup) {
        this->setup = setup;
    }else{
        //todo KILL execution, we can not continue without setup module;
        this->setup = nullptr;
    }
    if (stats) {
        if (setup->isServer()){
            this->s_stats = (cServerStats*)stats;
            this->c_stats = nullptr;
        }
        if (setup->isClient()){
            this->s_stats = nullptr;
            this->c_stats = (cClientStats*)stats;
        }
    }else{
        this->s_stats = nullptr;
        this->c_stats = nullptr;
    }
    dcnt_rx = 0;
    dcnt_tx = 0;

    this->rtt_avg = -1;
    this->rtt_min = -1;
    this->rtt_max = -1;
    this->pkt_rcvd = 0;
    this->pkt_sent = 0;
    this->last_seq_rcv = 0;
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
        this->sampled_int[i].bytes = 0;
    }
    json_first = true;
}


cMessageBroker::~cMessageBroker(){

}

void cMessageBroker::push(t_conn *conn, gen_msg_t *msg){
    struct t_msg_t *t_msg;
    t_msg = new t_msg_t;
    t_msg->tp = std::chrono::system_clock::now();
    t_msg->msg = msg;
    t_msg->conn = conn;
    msg_buf.push(t_msg);
    dcnt++;
}

void cMessageBroker::push_rx(gen_msg_t *msg){
    struct t_msg_t *t_msg;
    t_msg = new t_msg_t;
    t_msg->tp = std::chrono::system_clock::now();
    t_msg->msg = msg;
    msg_buf_rx.push(t_msg);
    dcnt_rx++;
}

void cMessageBroker::push_tx(gen_msg_t *msg){
    struct t_msg_t *t_msg;
    t_msg = new t_msg_t;
    t_msg->tp = std::chrono::system_clock::now();
    t_msg->msg = msg;
    msg_buf_tx.push(t_msg);
    dcnt_tx++;
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


void cMessageBroker::run(){
    u_int64_t rx_ts;
    u_int64_t tx_ts;
    t_msg_t * tmp_msg;
    t_msg_t * rx_msg;
    t_msg_t * tx_msg;
    u_int64_t first=0;
    u_int64_t last=0;
    if (setup->isClient()){
        while (!setup->isDone()){
            usleep(10);
            //HI priority message queue
            while (msg_buf_hp.front()){
                processAndDeleteClientMessage(*msg_buf_hp.front());
                delete *msg_buf_hp.front();
                msg_buf_hp.pop();
            }
            while ((msg_buf_rx.front())||(msg_buf_tx.front())){
                //std::cerr << msg_buf_rx.size() << std::endl;
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
                    last = tx_ts;
                    processAndDeleteClientMessage(*msg_buf_tx.front());
                    delete *msg_buf_tx.front();
                    msg_buf_tx.pop();
                }
                if ((rx_ts >= tx_ts) || ((tx_ts == 0)&&(rx_ts))){
                    processAndDeleteClientMessage(*msg_buf_rx.front());
                    delete *msg_buf_rx.front();
                    msg_buf_rx.pop();
                }
                //usleep(1);
            }
            //LOW priority message queue
            while (msg_buf_lp.front()){
                processAndDeleteClientMessage(*msg_buf_lp.front());
                delete *msg_buf_lp.front();
                msg_buf_lp.pop();
            }
        }
        //Todo: better signaling needded (do not shutdown mbroker until everything is done)
        //std::cout << "Done." << std::endl;
        while (msg_buf_lp.front()){
            processAndDeleteClientMessage(*msg_buf_lp.front());
            delete *msg_buf_lp.front();
            msg_buf_lp.pop();
        }
        //std::cerr << "MSG processed TX/RX: " << dcnt_tx << "/" << dcnt_rx << std::endl;
        //std::cerr << "Packet generator was active for [tv_sec]:  " << (last-first)/1000000000.0 << std::endl;
    }
    if (setup->isServer()){
        while (!setup->isDone()){
            usleep(10);
            //HI priority message queue
            while (msg_buf_hp.front()){
                processAndDeleteServerMessage(*msg_buf_hp.front());
                //delete *msg_buf_hp.front();
                msg_buf_hp.pop();
            }
            //MAIN queue
            while (msg_buf.front()){
                processAndDeleteServerMessage(*msg_buf.front());
                //delete *msg_buf.front();
                msg_buf.pop();
            }
            //LOW priority message queue
            while (msg_buf_lp.front()){
                processAndDeleteServerMessage(*msg_buf_lp.front());
                //delete *msg_buf_lp.front();
                msg_buf_lp.pop();
            }
        }
    }
}



void cMessageBroker::processAndDeleteServerMessage(t_msg_t *tmsg) {
    std::chrono::system_clock::time_point tp = tmsg->tp;
    gen_msg_t *msg = tmsg->msg;
    u_int64_t ts;
    switch(msg->type){
        case MSG_RX_PKT:
            break;
        case MSG_TX_PKT:
            break;
        case MSG_KEEP_ALIVE:
            //std::cout << "MSG_KEEP_ALIVE" << std::endl;
            break;
        case MSG_TIMER_ONE:
            //std::cout << "MSG_TIMER_ONE" << std::endl;
            break;
        default:
            std::cerr << "ERROR :: UNKNOWN MSG TYPE RECEIVED!";
    }
    delete msg;
    delete tmsg;
}

void cMessageBroker::processAndDeleteClientMessage(t_msg_t *tmsg){
    std::chrono::system_clock::time_point tp = tmsg->tp;
    gen_msg_t *msg = tmsg->msg;
    u_int64_t ts;

//    if (msg == nullptr){
//        std::cout << "MSG_NULL" << std::endl;
//        return;
//    }

    switch(msg->type){
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
            ts = (tp.time_since_epoch().count() * ((chrono::system_clock::period::num * 1000000000L) / chrono::system_clock::period::den));
            pkt_rtt = ((tp.time_since_epoch().count() * ((chrono::system_clock::period::num * 1000000000L) / chrono::system_clock::period::den)) - (ping_pkt->sec * 1000000000L) - (ping_pkt->nsec))/1000000.0; //ms
            if (not setup->silent()) {
                *output << prepDataRec(ts, RX, ping_pkt->size, ping_pkt->seq, pkt_rtt);
            }
            //c_stats->pktRecv(ts, ping_pkt->size, ping_pkt->seq, pkt_rtt);
            break;

        case MSG_TX_PKT:
            ping_pkt = (struct ping_pkt_t*) (msg);
            ts = (tp.time_since_epoch().count() * ((chrono::system_clock::period::num * 1000000000L) / chrono::system_clock::period::den));
            if (not setup->silent()) {
                *output << prepDataRec(ts, TX, ping_pkt->size, ping_pkt->seq, 0);
            }
            //c_stats->pktSent(ts, ping_pkt->size, ping_pkt->seq);
            break;
        case MSG_KEEP_ALIVE:
            //std::cout << "MSG_KEEP_ALIVE" << std::endl;
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
            ts = (tp.time_since_epoch().count() * ((chrono::system_clock::period::num * 1000000000L) / chrono::system_clock::period::den));
            //Flush buffered data
            if (not setup->silent()) {
                if (setup->getSampleLen()){
                    *output << prepFinalDataRec(TX);
                    *output << prepFinalDataRec(RX);
                }
            }
            if (setup->toJSON()) {
                *output << "\n\t],";
                *output << "\n\t\"client_stats\": {";
                *output << "\n\t\t\"ooo\" :"<< ooo_cnt << ",";
                *output << "\n\t\t\"dup\" :"<< dup_cnt << ",";
                *output << "\n\t\t\"duration\" :"<< (std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count())/1000.0;
                *output << "\n\t}";
                *output << "\n}"<<std::endl;
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
    delete(msg);
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
        header << "\n\t\t\"slot_duration\":" << setup->getSampleLen()/1000000000.0;
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



// double pkt_rtt, sample_cum_rtt;
// u_int64_t sample_seq_first, sample_pkt_cnt;
// u_int64_t sample_ts_limit;

std::string cMessageBroker::prepDataRec(const u_int64_t ts, const u_int8_t dir, const uint16_t size, const uint64_t seq, const float rtt){
    stringstream ss;
    if (setup->getSampleLen()){
        if (json_first){
            std::chrono::system_clock::time_point tp;
            json_first = false;
            sampled_int[RX].ts_limit = sampled_int[TX].ts_limit = setup->getSampleLen() + ts;
            ss << "\n\t\t{";
            jt_rtt = rtt;
            jt_rtt_prev = rtt;
        }
        if (ts < sampled_int[dir].ts_limit){
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
                sampled_int[dir].rtt_sum += rtt;
                jt_diff = rtt - jt_rtt_prev;
                if (jt_diff < 0) jt_diff = -jt_diff;
                jitter +=  (1.0/16.0) * (jt_diff - jitter);
                sampled_int[dir].jitter_sum += jitter;
            }
            sampled_int[dir].pkt_cnt++;
            sampled_int[dir].bytes += size;
            sampled_int[dir].last_seen_seq = seq;
            return ss.str();
        }else{
            sampled_int[dir].seq++;
            sampled_int[dir].ts_limit += setup->getSampleLen();
            if (sampled_int[dir].first == false){
                ss << ",\n\t\t{";
            }else{
                sampled_int[TX].first = false;
                sampled_int[RX].first = false;
            }
            if (ts < sampled_int[dir].ts_limit){
                ss << "\n\t\t\t\"ts\":"  << std::setprecision(6) << std::fixed << (double)(ts/1000000000.0) << ",";
                if (dir == TX){
                    ss << "\n\t\t\t\"dir\":\"tx\",";
                }
                if (dir == RX){
                    ss << "\n\t\t\t\"dir\":\"rx\",";
                    ss << "\n\t\t\t\"loss\":\"" << 1.0 - (float)sampled_int[dir].pkt_cnt / (float)((seq)-sampled_int[dir].first_seq) << "\","; //in ms
                    ss << "\n\t\t\t\"rtt\":\"" << std::setprecision(3) << sampled_int[dir].rtt_sum/sampled_int[dir].pkt_cnt << "\","; //in ms
                    ss << "\n\t\t\t\"jitter\":\"" << std::setprecision(3) << sampled_int[dir].jitter_sum/sampled_int[dir].pkt_cnt << "\","; //in ms
                    ss << "\n\t\t\t\"ooo\":\"" << sampled_int[dir].ooo << "\",";
                    ss << "\n\t\t\t\"dup\":\"" << sampled_int[dir].dup << "\",";
                }
                ss << "\n\t\t\t\"pkts\":\"" << sampled_int[dir].pkt_cnt << "\",";
                ss << "\n\t\t\t\"bytes\":" << sampled_int[dir].bytes << ",\n\t\t\t\"seq\":" << sampled_int[dir].seq << "\n\t\t}";
            }else{
                ss << "\n\t\t\t\"ts\":"  << std::setprecision(6) << std::fixed << (double)(ts/1000000000.0) << ",";
                if (dir == TX){
                    ss << "\n\t\t\t\"dir\":\"tx\",";
                }
                if (dir == RX){
                    ss << "\n\t\t\t\"dir\":\"rx\",";
                    ss << "\n\t\t\t\"loss\":\"" << 1 << "\",";
                    ss << "\n\t\t\t\"ooo\":\"" << 0 << "\",";
                    ss << "\n\t\t\t\"dup\":\"" << 0 << "\",";
                    ss << "\n\t\t\t\"rtt\":\"" << 0 << "\","; //in ms
                    ss << "\n\t\t\t\"jitter\":\"" << 0 << "\","; //in ms
                }
                ss << "\n\t\t\t\"pkts\":\"" << 0 << "\",";
                ss << "\n\t\t\t\"bytes\":" << 0 << ",\n\t\t\t\"seq\":" << sampled_int[dir].seq << "\n\t\t}";
            }
            sampled_int[dir].bytes = size;

            if (dir == RX) {
                sampled_int[dir].ooo = 0;
                sampled_int[dir].dup = 0;
                if (sampled_int[dir].last_seen_seq == seq){
                    dup_cnt++;
                    sampled_int[dir].dup++;
                    return ss.str();
                }
                if (sampled_int[dir].last_seen_seq > seq){
                    sampled_int[dir].ooo++;
                    ooo_cnt++;
                    return ss.str();
                }
                sampled_int[dir].rtt_sum = rtt;
                jt_diff = rtt - jt_rtt_prev;
                jt_rtt_prev = rtt;
                if (jt_diff < 0) jt_diff = -jt_diff;
                jitter +=  (1.0/16.0) * (jt_diff - jitter);
                sampled_int[dir].jitter_sum  = jitter;
            }
            sampled_int[dir].pkt_cnt = 1;
            sampled_int[dir].first_seq = seq;
            sampled_int[dir].last_seen_seq = seq;
            return ss.str();
        }
    } else{
        if (json_first){
            json_first = false;
            ss << "\n\t\t{";
        }else{
            ss << ",\n\t\t{";
        }
        ss << "\n\t\t\t\"ts\":"  << std::setprecision(6) << std::fixed << (double)(ts/1000000000.0) << ",";
        if (dir == TX){
            ss << "\n\t\t\t\"dir\":\"tx\",";
        }
        if (dir == RX){
            if (sampled_int[dir].last_seen_seq == seq){
                dup_cnt++;
                return ss.str();
            }
            if (sampled_int[dir].last_seen_seq > seq){
                ooo_cnt++;
                return ss.str();
            }

            ss << "\n\t\t\t\"dir\":\"rx\",";
            ss << "\n\t\t\t\"rtt\":\"" << std::setprecision(3) << rtt << "\","; //in ms
        }
        ss << "\n\t\t\t\"size\":" << size << ",\n\t\t\t\"seq\":" << seq << "\n\t\t}";
        sampled_int[dir].last_seen_seq = seq;
        return ss.str();
    }
    return ss.str();
}

std::string cMessageBroker::prepFinalDataRec(const u_int8_t dir){
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
        ss << "\n\t\t\t\"ts\":"  << std::setprecision(6) << std::fixed << (double)(sampled_int[dir].ts_limit/1000000000.0) << ",";
        if (dir == TX){
            ss << "\n\t\t\t\"dir\":\"tx\",";
        }
        if (dir == RX){
            ss << "\n\t\t\t\"dir\":\"rx\",";
            ss << "\n\t\t\t\"loss\":\"" << 1.0 - (float)sampled_int[dir].pkt_cnt / (float)((sampled_int[dir].last_seen_seq+1)-sampled_int[dir].first_seq) << "\","; //in ms
            ss << "\n\t\t\t\"rtt\":\"" << std::setprecision(3) << sampled_int[dir].rtt_sum/sampled_int[dir].pkt_cnt << "\","; //in ms
            ss << "\n\t\t\t\"jitter\":\"" << std::setprecision(3) << sampled_int[dir].jitter_sum/sampled_int[dir].pkt_cnt << "\","; //in ms
            ss << "\n\t\t\t\"ooo\":\"" << sampled_int[dir].ooo << "\",";
            ss << "\n\t\t\t\"dup\":\"" << sampled_int[dir].dup << "\",";
        }
        ss << "\n\t\t\t\"pkts\":\"" << sampled_int[dir].pkt_cnt << "\",";
        ss << "\n\t\t\t\"bytes\":" << sampled_int[dir].bytes << ",\n\t\t\t\"seq\":" << sampled_int[dir].seq << "\n\t\t}";
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
