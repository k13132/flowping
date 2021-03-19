/*
 * File:   cStats.cpp
 *
 * Copyright (C) 2016: Department of Telecommunication Engineering, FEE, CTU in Prague
 *
 * This file is part of FlowPing.
 *
 * FlowPing is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * FlowPing is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 *
 * You should have received a copy of the GNU General Public License
 * along with FlowPing.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ondrej Vondrous
 *         Department of Telecommunication Engineering
 *         FEE, Czech Technical University in Prague
 *         ondrej.vondrous@fel.cvut.cz
 */


#include "cStats.h"
#include "_types.h"
#include "iomanip"



using namespace std;

cStats::cStats() {
}

cStats::~cStats() {
}

void cStats::printRealTime(void) {
}

void cStats::printSummary(void) {
}

void cStats::pk_enque(const u_int64_t conn_id, const u_int16_t direction, const timespec ts, const u_int16_t len, const u_int64_t seq, const float rtt) {
    pinfo_t pk_info;
    pk_info.ts = ts;
    pk_info.len = len;
    pk_info.seq = seq;
    pk_info.rtt = rtt;
    queue<pinfo_t> * pk_queue;
    qstats_t * pk_stats;

    if (qstats.count(conn_id) == 0) {
        pk_stats = new qstats_t;
        pk_stats->rx_qsize = 0;
        pk_stats->tx_qsize = 0;
        pk_stats->rx_qtime = 0;
        pk_stats->tx_qtime = 0;
        pk_stats->rx_q_cumulative_rtt = 0;
        pk_stats->max_seq = 0;
        qstats[conn_id] = pk_stats;
    } else {
        pk_stats = (qstats_t *) qstats[conn_id];
    }

    if (direction == TX) {
        if (pk_info_tx_queues.count(conn_id) == 0) {
            pk_queue = new queue<pinfo_t>;
            pk_info_tx_queues[conn_id] = pk_queue;
        } else {
            pk_queue = (queue<pinfo_t> *) pk_info_tx_queues[conn_id];
        }
        if (pk_queue->size()) {
            pk_stats->tx_qtime = NS_TDIFF(pk_info.ts, pk_queue->front().ts);
        } else {
            pk_stats->tx_qtime = 1;
        }
        pk_stats->tx_qsize += pk_info.len;
        pk_queue->push(pk_info);
        while (pk_stats->tx_qtime > 10000000000 && pk_queue->size() > 2) {
            pk_info = pk_queue->front();
            pk_queue->pop();
            pk_stats->tx_qtime = NS_TDIFF(pk_queue->back().ts, pk_info.ts);
            pk_stats->tx_qsize -= pk_info.len;
        }
    }
    if (direction == RX) {
        if (pk_info_rx_queues.count(conn_id) == 0) {
            pk_queue = new queue<pinfo_t>;
            pk_info_rx_queues[conn_id] = pk_queue;
        } else {
            pk_queue = (queue<pinfo_t> *) pk_info_rx_queues[conn_id];
        }
        if (pk_queue->size()) {
            pk_stats->rx_qtime = NS_TDIFF(pk_info.ts, pk_queue->front().ts);
        } else {
            pk_stats->rx_qtime = 1;
        }
        pk_stats->rx_qsize += pk_info.len;
        pk_stats->max_seq = max(pk_stats->max_seq, seq);
        pk_stats->rx_q_cumulative_rtt += (u_int64_t) (pk_info.rtt * 1000.0);
        pk_queue->push(pk_info);
        while (pk_stats->rx_qtime > 10000000000 && pk_queue->size() > 2) {
            pk_info = pk_queue->front();
            pk_queue->pop();
            pk_stats->rx_qtime = NS_TDIFF(pk_queue->back().ts, pk_info.ts);
            pk_stats->rx_qsize -= pk_info.len;
            pk_stats->rx_q_cumulative_rtt -= (u_int64_t) (pk_info.rtt * 1000.0);
        }
    }
}

void cStats::prepareStats(const u_int64_t conn_id, const uint16_t direction, stats_t* stats) {
    queue<pinfo_t> * pk_queue;
    qstats_t * pk_stats;
    pk_stats = nullptr;
    pk_queue = nullptr;
    u_int32_t max_seq;
    if (qstats.count(conn_id) == 0) {
        cerr << "cStats::ERRROR (1) - no stats for connId: " << conn_id << endl;
    } else {
        pk_stats = (qstats_t *) qstats[conn_id];
    }
    if (direction == TX) {
        if (pk_info_tx_queues.count(conn_id) == 0) {
            cerr << "cStats::ERRROR (2) - no stats for connId: " << conn_id << endl;
        } else {
            pk_queue = (queue<pinfo_t> *) pk_info_tx_queues[conn_id];
        }
        stats->tx_bitrare = (pk_stats->tx_qsize * 8000000000) / pk_stats->tx_qtime;
        if (!setup->isServer()) {
            ((c_stats_t *) stats)->tx_pps = pk_queue->size() * 1000000000 / (pk_stats->tx_qtime);
        }
    }
    if (direction == RX) {
        if (pk_info_rx_queues.count(conn_id) == 0) {
            cerr << "cStats::ERRROR (3) - no stats for connId: " << conn_id << endl;
        } else {
            pk_queue = (queue<pinfo_t> *) pk_info_rx_queues[conn_id];
        }
        stats->rx_bitrare = (pk_stats->rx_qsize * 8000000000) / pk_stats->rx_qtime;
        if (!setup->isServer()) {
            ((c_stats_t *) stats)->cur_rtt = (float) ((double) (pk_stats->rx_q_cumulative_rtt) / ((float) pk_queue->size()*1000));
        }
        max_seq = max(pk_queue->back().seq, pk_stats->max_seq);
        stats->cur_loss = 100.0 * ((float) (1 + (max_seq - pk_queue->front().seq) - pk_queue->size()) / (float) (max_seq - pk_queue->front().seq));
        stats->rx_pps = pk_queue->size() * 1000000000 / (pk_stats->rx_qtime);
    }
}

cClientStats::cClientStats(cSetup *setup) {
    timespec curTv;
    clock_gettime(CLOCK_REALTIME, &curTv);
    stats = new c_stats_t;
    stats->tx_bitrare = 0;
    stats->rx_bitrare = 0;
    stats->dst = "";
    stats->rtt_avg = 0;
    stats->rtt_max = 0;
    stats->rtt_min = INT_MAX;
    stats->rx_pkts = 0;
    stats->tx_pkts = 0;
    stats->ooo_pkts = 0;
    stats->server_rx_pkts = 0;
    stats->rx_loss = 0;
    stats->tx_bytes = 0;
    stats->rx_bytes = 0;
    stats->test_start = NS_TIME(curTv);
    stats->cur_loss = 0;
    stats->cur_rtt = 0;
    stats->cumulative_rtt = 0;
    this->setup = setup;
}

cClientStats::~cClientStats(){
    delete this->stats;
}


std::ostream& operator<<(std::ostream& os, const timed_packet_t& obj)
{
    os << "ts:" << obj.sec << "." << std::setfill('0') << std::setw(3) << (obj.nsec/1000000) << " length:"<<obj.len;
    return os;
}

//std::ostream& operator<<(std::ostream& os, const ts_t& obj)
//{
//    os << "ts:" << obj.tv_sec << "." << std::setfill('0') << std::setw(9) << obj.tv_nsec;
//    return os;
//}

std::ostream& operator<<(std::ostream& os, const timespec& obj)
{
    os << "ts:" << obj.tv_sec << "." << std::setfill('0') << std::setw(9) << obj.tv_nsec;
    return os;
}

void cStats::printStatus() {
    std::cout << "status: " << status << std::endl;
}

void cStats::setStatus(std::string status) {
    this->status = status;
}


void cClientStats::printRealTime(void) {
    std::stringstream ss;
    timespec curTv;
    double duration;
    this->prepareStats(1, RX, stats);
    this->prepareStats(1, TX, stats);
    this->stats->rtt_avg = (float) ((double) this->stats->cumulative_rtt / (float) (this->stats->rx_pkts * 1000));

    clock_gettime(CLOCK_REALTIME, &curTv);
    duration = (double) (NS_TIME(curTv)-(stats->test_start)) / 1000000000.0;
    //timestamp;hostname;test_duration;tx_pkts;rx_pkts;pk_loss;ooo_pkts;bytes_sent;bytes_received;avg_bitrate_tx, avg_bitrate_rx;avg_rtt;avg_pk_losscurrent_bitrate_tx;current_bitrate_rx;current_rtt;current_pk_loss


    if (setup->toJSON()) {
        ss.str("");
        ss << "{\"ts\":" << curTv.tv_sec << ".";
        ss.fill('0');
        ss.width(9);
        ss << curTv.tv_nsec << ",";
        ss << "\"remote_host\":\"" << setup->getHostname() << "\",";
        ss << "\"life_time_stats\":{";
        ss << "\"duration\":" << duration << ",";
        ss << "\"pkt_buffer_fill\":" << setup->getTimedBufferSize() << ",";
        ss << "\"last_timed_pkt_info\":" << setup->get_tmp_tpck() << ",";
        ss << "\"last_delay\":" << setup->getLastDelay() << ",";
        ss.precision(3);
        ss.fill('0');
        ss.width(6);
        ss.setf(ios_base::right, ios_base::adjustfield);
        ss.setf(ios_base::fixed, ios_base::floatfield);
        ss << "\"rtt_min\":" << stats->rtt_min << ",";
        ss << "\"rtt_avg\":" << stats->rtt_avg << ",";
        ss << "\"rtt_max\":" << stats->rtt_max << ",";
        ss.precision(0);
        ss << "\"tx_pkts\":" << stats->tx_pkts << ",";
        ss << "\"rx_pkts\":" << stats->rx_pkts << ",";
        ss << "\"tx_bytes\":" << stats->tx_bytes << ",";
        ss << "\"rx_bytes\":" << stats->rx_bytes << ",";
        ss << "\"tx_bitrate\":" << stats->tx_bytes * 8 / (duration * 1000) << ",";
        ss << "\"rx_bitrate\":" << stats->rx_bytes * 8 / (duration * 1000) << ",";
        ss << "\"tx_pkt_rate\":" << stats->tx_pkts / duration << ",";
        ss << "\"rx_pkt_rate\":" << stats->rx_pkts / duration << "},";
        ss << "\"live_stats\":{";
        ss << "\"tx_bitrate\":" << stats->tx_bitrare / 1000 << ",";
        ss << "\"rx_bitrate\":" << stats->rx_bitrare / 1000 << ",";
        ss << "\"tx_pkt_rate\":" << stats->tx_pps << ",";
        ss << "\"rx_pkt_rate\":" << stats->rx_pps << ",";
        ss.precision(3);
        ss << "\"rtt\":" << stats->cur_rtt << ",";
        ss << "\"loss\":" << stats->cur_loss << "}}";
        std::cout << ss.str() << endl;
    }
    if (setup->toCSV()) {

        ss.str("");
        ss << curTv.tv_sec << ";";
        ss.fill('0');
        ss.width(9);
        ss << curTv.tv_nsec << ";";
        ss << setup->getHostname() << ";";
        ss << duration << ";";
        ss.precision(3);
        ss.fill('0');
        ss.width(6);
        ss.setf(ios_base::right, ios_base::adjustfield);
        ss.setf(ios_base::fixed, ios_base::floatfield);
        ss << stats->rtt_min << ";";
        ss << stats->rtt_avg << ";";
        ss << stats->rtt_max << ";";
        ss.precision(0);
        ss << stats->tx_pkts << ";";
        ss << stats->rx_pkts << ";";
        ss << stats->tx_bytes << ";";
        ss << stats->rx_bytes << ";";
        ss << stats->tx_bytes * 8 / (duration * 1000) << ";";
        ss << stats->rx_bytes * 8 / (duration * 1000) << ";";
        ss << stats->tx_pkts / duration << ";";
        ss << stats->rx_pkts / duration << ";";
        ss << stats->tx_bitrare / 1000 << ";";
        ss << stats->rx_bitrare / 10000 << ";";
        ss.precision(3);
        ss << stats->cur_rtt << ";";
        ss << stats->cur_loss << ";";
        ss << stats->tx_pps << ";";
        ss << stats->rx_pps << ";";
        std::cout << ss.str() << endl;
    }
    if (!setup->toCSV() && !setup->toJSON()) {
        ss.str("");
        ss << "=====================================\n";
        ss << "remote host: " << setup->getHostname() << "\n";
        ss << "ts: " << curTv.tv_sec << ".";
        ss.fill('0');
        ss.width(9);
        ss << curTv.tv_nsec << " [s]\n";
        ss << "\n>>> LIFE TIME STATS" << std::endl;
        ss << "test_duration [s]: " << duration << std::endl;
        ss << "pkt_buffer_fill [pkts]:" << setup->getTimedBufferSize() << std::endl;
        ss << "last timed packet info:" << setup->get_tmp_tpck() << std::endl;
        ss.precision(3);
        ss.fill('0');
        ss.width(6);
        ss.setf(ios_base::right, ios_base::adjustfield);
        ss.setf(ios_base::fixed, ios_base::floatfield);
        ss << "min_rtt: " << stats->rtt_min << " [ms]" << std::endl;
        ss << "avg_rtt: " << stats->rtt_avg << " [ms]" << std::endl;
        ss << "max_rtt: " << stats->rtt_max << " [ms]" << std::endl;
        ss.precision(0);
        ss << "tx_packets: " << stats->tx_pkts << std::endl;
        ss << "rx_packets: " << stats->rx_pkts << std::endl;
        ss << "tx_bytes: " << stats->tx_bytes << " [B]" << std::endl;
        ss << "rx_bytes: " << stats->rx_bytes << " [B]" << std::endl;
        ss << "tx_bitrate: " << stats->tx_bytes * 8 / (duration * 1000) << " [kbps]" << std::endl;
        ss << "rx_bitrate: " << stats->rx_bytes * 8 / (duration * 1000) << " [kbps]" << std::endl;
        ss << "tx_pkt_rate: " << stats->tx_pkts / duration << " [pps]" << std::endl;
        ss << "rx_pkt_rate: " << stats->rx_pkts / duration << " [pps]" << std::endl;
        ss << "\n>>> LIVE STATS (last 10 tv_sec)" << std::endl;
        ss << "tx_bitrate: " << stats->tx_bitrare / 1000 << " [kbps]" << std::endl;
        ss << "rx_bitrate: " << stats->rx_bitrare / 1000 << " [kbps]" << std::endl;
        ss.precision(3);
        ss << "rtt: " << stats->cur_rtt << " [ms]" << std::endl;
        ss << "loss: " << stats->cur_loss << " [%]" << std::endl;
        ss << "tx_pkt_rate: " << stats->tx_pps << " [pps]" << std::endl;
        ss << "rx_pkt_rate: " << stats->rx_pps << " [pps]" << std::endl;
        ss << "=====================================\n";
        std::cout << ss.str() << endl;
    }
}

void cClientStats::printSummary(void) {

}

void cClientStats::addServerStats(const u_int64_t server_rx_pkts) {
    stats->server_rx_pkts = server_rx_pkts;
}

std::string cClientStats::getReport() {
    timespec curTv;
    clock_gettime(CLOCK_REALTIME, &curTv);

    this->prepareStats(1, RX, stats);
    this->prepareStats(1, TX, stats);
    this->stats->rtt_avg = (float) ((double) this->stats->cumulative_rtt / (float) (this->stats->rx_pkts * 1000));
    float p = (100.0 * (stats->tx_pkts - stats->rx_pkts)) / stats->tx_pkts;
    float uloss = (float) (100.0 * (stats->tx_pkts - stats->server_rx_pkts)) / stats->tx_pkts;
    float dloss = (float) (100.0 * (stats->server_rx_pkts - stats->rx_pkts)) / stats->server_rx_pkts;

    std::stringstream ss;
    ss.str("");
    ss.setf(std::ios_base::right, std::ios_base::adjustfield);
    ss.setf(std::ios_base::fixed, std::ios_base::floatfield);
    ss.precision(2);

    if (!setup->toJSON()) {
        ss << "\n---Client report--- " << setup->getHostname() << " ping statistics ---\n";
        ss << stats->tx_pkts << " packets transmitted, " << stats->rx_pkts << " received, ";
        ss << p << "% packet loss, time ";
        ss << (NS_TIME(curTv) - stats->test_start) / 1000000 << "ms\n";

        ss << "rtt min/avg/max = ";
        ss << stats->rtt_min << "/" << stats->rtt_avg << "/" << stats->rtt_max << " ms ";
        ss << "Out of Order = " << stats->ooo_pkts << " packets\n";

        ss << "\n---Server report--- " << setup->getHostname() << " ping statistics ---\n";
        ss << stats->server_rx_pkts << " received, ";
        ss << uloss << "% upstream packet loss, ";
        ss << dloss << "% downstream packet loss\n\n";
    }
    if (setup->toJSON()) {
        if (!setup->silent()){
            ss << "\n\t],\n";
        }
        ss << "\t\"client_stats\":{\n\t\t\"host\":\""<< setup->getHostname() << "\",\n\t\t\"tx_pkts\":\""<< stats->tx_pkts << "\"";
        ss << ",\n\t\t\"rx_pkts\":\""<< stats->rx_pkts << "\",\n\t\t\"loss\":\""<< p << "\",\n\t\t\"duration\":\""<< (NS_TIME(curTv) - stats->test_start) / 1000000 << "\"";
        ss << ",\n\t\t\"rtt_min\":\""<<stats->rtt_min << "\",\n\t\t\"rtt_avg\":\"" << stats->rtt_avg << "\",\n\t\t\"rtt_max\":\""<< stats->rtt_max << "\"";
        ss << ",\n\t\t\"out of order\":\"" << stats->ooo_pkts << "\"\n\t},\n";

        ss << "\t\"server_stats\":{\n\t\t\"host\":\""<< setup->getHostname() << "\",\n\t\t\"rx_pkts\":\""<< stats->server_rx_pkts << "\"";
        ss << ",\n\t\t\"up_loss\":\""<< uloss << "\"";
        ss << ",\n\t\t\"down_loss\":\""<< dloss << "\"\n\t}" << std::endl;
    }
    
    return ss.str();
}

void cClientStats::pktRecv(const timespec ts, const u_int16_t len, const u_int64_t seq, const float rtt) {
    stats->rx_pkts++;
    stats->rx_bytes += len;
    stats->cumulative_rtt += (rtt * 1000);
    if (stats->rtt_min == -1) {
        stats->rtt_min = rtt;
        stats->rtt_max = rtt;
    } else {
        if (rtt < stats->rtt_min) {
            stats->rtt_min = rtt;
        }
        if (rtt > stats->rtt_max) {
            stats->rtt_max = rtt;
        }
    }
    //ToDo Rewrite to simplified form //cumulative rtt/rc_pkts
    this->pk_enque(1, RX, ts, len, seq, rtt);
}

void cClientStats::pktRecv(const u_int64_t ts, const u_int16_t len, const u_int64_t seq, const float rtt) {
    stats->rx_pkts++;
    stats->rx_bytes += len;
    stats->cumulative_rtt += (rtt * 1000);
    if (stats->rtt_min == -1) {
        stats->rtt_min = rtt;
        stats->rtt_max = rtt;
    } else {
        if (rtt < stats->rtt_min) {
            stats->rtt_min = rtt;
        }
        if (rtt > stats->rtt_max) {
            stats->rtt_max = rtt;
        }
    }
    //ToDo Rewrite to simplified form //cumulative rtt/rc_pkts
    timespec tv;
    tv.tv_sec = TV_SEC(ts);
    tv.tv_nsec = TV_NSEC(ts);
    this->pk_enque(1, RX, tv, len, seq, rtt);
}

void cClientStats::pktSent(const timespec ts, const uint16_t len, const u_int64_t seq) {
    stats->tx_pkts++;
    stats->tx_bytes += len;
    this->pk_enque(1, TX, ts, len, seq, 0);
}

void cClientStats::pktSent(const u_int64_t ts, const uint16_t len, const u_int64_t seq) {
    stats->tx_pkts++;
    stats->tx_bytes += len;
    timespec tv;
    tv.tv_sec = TV_SEC(ts);
    tv.tv_nsec = TV_NSEC(ts);
    this->pk_enque(1, TX, tv, len, seq, 0);
}

void cClientStats::pktOoo() {
    stats->ooo_pkts++;
}

cServerStats::cServerStats(cSetup *setup) {
    if (setup) {
        this->setup = setup;
    }else{
        this->setup = nullptr;
    }
}

cServerStats::~cServerStats(){
    //delete this->stats;
}


void cServerStats::pktSent(const u_int64_t conn_id, const timespec ts, const uint16_t len, const u_int64_t seq, const std::string src, const u_int32_t port) {
    if (s_stats.count(conn_id) == 0) {
        //cerr << "ServerStats {stats} should be initialized at the time of instance construction" << endl;
        //exit(1);
        connInit(conn_id, src, port);
    }
    stats = s_stats[conn_id];
    stats->tx_bytes += len;
    this->pk_enque(conn_id, TX, ts, len, seq, 0);
}

void cServerStats::pktReceived(const u_int64_t conn_id, const timespec ts, const uint16_t len, const u_int64_t seq, const std::string src, const u_int32_t port) {
    if (s_stats.count(conn_id) == 0) {
        // in case of server restart and client test still running.
        connInit(conn_id, src, port);
    }
    stats = s_stats[conn_id];
    stats->rx_pkts++;
    stats->rx_bytes += len;
    stats->max_seq = max(stats->max_seq, seq);
    this->pk_enque(conn_id, RX, ts, len, seq, 0);
}

void cServerStats::printRealTime(void) {
    std::stringstream ss;
    timespec curTv;
    double duration;
    clock_gettime(CLOCK_REALTIME, &curTv);

    for (std::map<u_int64_t, s_stats_t *>::const_iterator it = s_stats.begin(); it != s_stats.end(); ++it) {
        this->prepareStats(it->first, RX, it->second);
        this->prepareStats(it->first, TX, it->second);
        duration = (double) (NS_TIME(curTv)-(it->second->test_start)) / 1000000000.0;
        if (setup->toJSON()) {
            ss.str("");
            ss << "{\"ts\":" << curTv.tv_sec << ".";
            ss.fill('0');
            ss.width(9);
            ss << curTv.tv_nsec << ",";
            ss << "\"remote\":\"" << it->second->src << ":" << it->second->port << "\",";
            ss << "\"life_time_stats\":{";
            ss << "\"duration\":" << duration << ",";
            ss.precision(3);
            ss.fill('0');
            ss.width(6);
            ss.setf(ios_base::right, ios_base::adjustfield);
            ss.setf(ios_base::fixed, ios_base::floatfield);
            ss.precision(0);
            ss << "\"rxtx_pkts\":" << it->second->rx_pkts << ",";
            ss << "\"tx_bytes\":" << it->second->tx_bytes << ",";
            ss << "\"rx_bytes\":" << it->second->rx_bytes << ",";
            ss << "\"tx_bitrate\":" << it->second->tx_bytes * 8 / duration << ",";
            ss << "\"rx_bitrate\":" << it->second->rx_bytes * 8 / duration << ",";
            ss << "\"rxtx_pkt_rate\":" << it->second->rx_pkts / duration << ",";
            ss << "\"rx_loss\":" << it->second->max_seq - it->second->rx_pkts<< "},";
            ss << "\"live_stats\":{";
            ss << "\"tx_bitrate\":" << it->second->tx_bitrare << ",";
            ss << "\"rx_bitrate\":" << it->second->rx_bitrare << ",";
            ss << "\"rxtx_pkt_rate\":" << it->second->rx_pps << ",";
            ss.precision(3);
            ss << "\"loss\":" << it->second->cur_loss << "}}";
            std::cout << ss.str() << endl;
        }
        if (setup->toCSV()) {

            ss.str("");
            ss << curTv.tv_sec << ";";
            ss.fill('0');
            ss.width(9);
            ss << curTv.tv_nsec << ";";
            ss << it->second->src << ";";
            ss << it->second->port << ";";
            ss << duration << ";";
            ss.precision(3);
            ss.fill('0');
            ss.width(6);
            ss.setf(ios_base::right, ios_base::adjustfield);
            ss.setf(ios_base::fixed, ios_base::floatfield);
            ss.precision(0);
            ss << it->second->rx_pkts << ";";
            ss << it->second->tx_bytes << ";";
            ss << it->second->rx_bytes << ";";
            ss << it->second->tx_bytes * 8 / duration << ";";
            ss << it->second->rx_bytes * 8 / duration << ";";
            ss << it->second->rx_pkts / duration << ";";
            ss << it->second->rx_pkts - it->second->max_seq << ";";
            ss << it->second->tx_bitrare << ";";
            ss << it->second->rx_bitrare << ";";
            ss << it->second->rx_pps << ";";
            ss.precision(3);
            ss << it->second->cur_loss << ";";
            std::cout << ss.str() << endl;
        }
        if (!setup->toCSV() && !setup->toJSON()) {
            ss.str("");
            ss << "=====================================\n";
            ss << "remote: " << it->second->src << ":" << it->second->port << "\n";
            ss << "ts: " << curTv.tv_sec << ".";
            ss.fill('0');
            ss.width(9);
            ss << curTv.tv_nsec << " [s]\n";
            ss << "\n>>> LIFE TIME STATS" << std::endl;
            ss << "test_duration [s]: " << duration << std::endl;
            ss.precision(3);
            ss.fill('0');
            ss.width(6);
            ss.setf(ios_base::right, ios_base::adjustfield);
            ss.setf(ios_base::fixed, ios_base::floatfield);
            ss.precision(0);
            ss << "rxtx_packets: " << it->second->rx_pkts << std::endl;
            ss << "tx_bytes: " << it->second->tx_bytes << " [B]" << std::endl;
            ss << "rx_bytes: " << it->second->rx_bytes << " [B]" << std::endl;
            ss << "tx_bitrate: " << it->second->tx_bytes * 8 / duration << " [bps]" << std::endl;
            ss << "rx_bitrate: " << it->second->rx_bytes * 8 / duration << " [bps]" << std::endl;
            ss << "rxtx_pkt_rate: " << it->second->rx_pkts / duration << " [pps]" << std::endl;
            ss << "rx_loss: " << it->second->max_seq - it->second->rx_pkts << std::endl;
            ss << "\n>>> LIVE STATS (last 10 tv_sec)" << std::endl;
            ss << "tx_bitrate: " << it->second->tx_bitrare << " [bps]" << std::endl;
            ss << "rx_bitrate: " << it->second->rx_bitrare << " [bps]" << std::endl;
            ss << "rxtx_pkt_rate: " << it->second->rx_pps << " [pps]" << std::endl;
            ss.precision(3);
            ss << "loss: " << it->second->cur_loss << " [%]" << std::endl;
            ss << "=====================================\n";
            std::cout << ss.str() << endl;
        }
    }

}

void cServerStats::printSummary(void) {

}

u_int16_t cServerStats::connStatRemove(const u_int64_t conn_id) {
    if (s_stats.count(conn_id) > 0) {
        if (s_stats[conn_id]) {
            delete s_stats[conn_id];
        }
        s_stats.erase(conn_id);
        return 0;
    }
    return 1;
}

void cServerStats::connInit(const u_int64_t conn_id, const string src, const u_int32_t port) {
    timespec curTv;
    clock_gettime(CLOCK_REALTIME, &curTv);

    if (s_stats.count(conn_id) == 0) {
        stats = new s_stats_t;
        s_stats[conn_id] = stats;
    }
    stats = s_stats[conn_id];
    stats->tx_bitrare = 0;
    stats->rx_bitrare = 0;
    stats->max_seq = 0;
    stats->src = src;
    stats->port = port;
    stats->rx_pkts = 0;
    stats->rx_loss = 0;
    stats->ooo_pkts = 0;
    stats->tx_bytes = 0;
    stats->rx_bytes = 0;
    stats->duration = 0;
    stats->test_start = NS_TIME(curTv);
}
