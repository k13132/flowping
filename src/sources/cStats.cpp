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


#define NS_TDIFF(tv1,tv2) ( ((tv1).tv_sec-(tv2).tv_sec)*1000000000 +  ((tv1).tv_nsec-(tv2).tv_nsec) )

cStats::cStats() {
}

cStats::~cStats() {
}

void cStats::printRealTime(void) const {
}

void cStats::printSummary(void) const {
}

u_int32_t cStats::pk_enque(const u_int64_t conn_id, const u_int16_t direction, const timespec ts, const u_int16_t len) {
    pinfo_t pk_info;
    pk_info.ts = ts;
    pk_info.len = len;
    queue<pinfo_t> * pk_queue;
    qstats_t * pk_stats;

    if (qstats.count(conn_id) == 0) {
        pk_stats = new qstats_t;
        pk_stats->rx_qsize=0;
        pk_stats->tx_qsize=0;
        pk_stats->rx_qtime=0;
        pk_stats->tx_qtime=0;
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
        return (pk_stats->tx_qsize * 8000000000) / pk_stats->tx_qtime;
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
        pk_queue->push(pk_info);
        while (pk_stats->rx_qtime > 10000000000 && pk_queue->size() > 2) {
            pk_info = pk_queue->front();
            pk_queue->pop();
            pk_stats->rx_qtime = NS_TDIFF(pk_queue->back().ts, pk_info.ts);
            pk_stats->rx_qsize -= pk_info.len;
        }
        //cout << pk_stats->rx_qtime << "\t" << pk_stats->rx_qsize << endl;
        return (pk_stats->rx_qsize * 8000000000) / pk_stats->rx_qtime;
    }
    return 0;
}

cClientStats::cClientStats(cSetup *setup) {
    stats.tx_bitrare = 0;
    stats.rx_bitrare = 0;
    stats.dst = "";
    stats.rtt_avg = 0;
    stats.rtt_max = 0;
    stats.rtt_min = INT_MAX;
    stats.rx_pkts = 0;
    stats.tx_pkts = 0;
    stats.ooo_pkts = 0;
    stats.server_rx_pkts = 0;
    this->setup = setup;
}

void cClientStats::printRealTime(void) const {
    std::stringstream ss;
    timespec curTv;

    clock_gettime(CLOCK_REALTIME, &curTv);
    ss.str("");
    ss << "cClientStats:\n";

    ss << "ts: " << curTv.tv_sec << ".";
    ss.fill('0');
    ss.width(9);
    ss << curTv.tv_nsec;
    ss << "\n";

    ss.precision(3);
    ss.fill('0');
    ss.width(6);
    ss.setf(ios_base::right, ios_base::adjustfield);
    ss.setf(ios_base::fixed, ios_base::floatfield);
    ss << "min_rtt: " << stats.rtt_min << std::endl;
    ss << "avg_rtt: " << stats.rtt_avg << std::endl;
    ss << "max_rtt: " << stats.rtt_max << std::endl;
    ss.precision(0);
    ss << "tx_packets: " << stats.tx_pkts << std::endl;
    ss << "rx_packets: " << stats.rx_pkts << std::endl;
    ss << "tx_bitrate: " << stats.tx_bitrare << std::endl;
    ss << "rx_bitrate: " << stats.rx_bitrare << std::endl;
    std::cout << ss.str() << endl;
}

void cClientStats::printSummary(void) const {

}

void cClientStats::addServerStats(const u_int64_t server_rx_pkts) {
    stats.server_rx_pkts = server_rx_pkts;
}

std::string cClientStats::getReport() const {
    std::stringstream ss;
    float p = (100.0 * (stats.tx_pkts - stats.rx_pkts)) / stats.tx_pkts;
    float uloss = (float) (100.0 * (stats.tx_pkts - stats.server_rx_pkts)) / stats.tx_pkts;
    float dloss = (float) (100.0 * (stats.server_rx_pkts - stats.rx_pkts)) / stats.server_rx_pkts;

    ss.str("");
    ss.setf(std::ios_base::right, std::ios_base::adjustfield);
    ss.setf(std::ios_base::fixed, std::ios_base::floatfield);
    ss.precision(2);

    ss << "\n---Client report--- " << setup->getHostname() << " ping statistics ---\n";


    ss << stats.tx_pkts << " packets transmitted, " << stats.rx_pkts << " received, ";
    ss << p << "% packet loss, time ";
    ss << stats.duration << "ms\n";

    ss << "rtt min/avg/max = ";
    ss << stats.rtt_min << "/" << stats.rtt_avg << "/" << stats.rtt_max << " ms ";
    ss << "Out of Order = " << stats.ooo_pkts << " packets\n";

    ss << "\n---Server report--- " << setup->getHostname() << " ping statistics ---\n";
    ss << stats.server_rx_pkts << " received, ";
    ss << uloss << "% upstream packet loss, ";
    ss << dloss << "% downstream packet loss\n\n";
    return ss.str();
}

void cClientStats::addCRxInfo(const timespec ts, const u_int16_t len, const double rtt) {
    stats.rx_pkts++;
    if (stats.rtt_min == -1) {
        stats.rtt_min = rtt;
        stats.rtt_max = rtt;
        stats.rtt_avg = rtt;
    } else {
        if (rtt < stats.rtt_min) {
            stats.rtt_min = rtt;
        }
        if (rtt > stats.rtt_max) {
            stats.rtt_max = rtt;
        }
    }
    stats.rtt_avg = ((stats.rx_pkts - 1) * stats.rtt_avg + rtt) / stats.rx_pkts;
    stats.rx_bitrare = this->pk_enque(1, RX, ts, len);
}

void cClientStats::pktSent(const timespec ts, const uint16_t len) {
    stats.tx_pkts++;
    stats.tx_bitrare = this->pk_enque(1, TX, ts, len);
}

void cClientStats::pktOoo() {
    stats.ooo_pkts++;
}

cServerStats::cServerStats(cSetup *setup) {
    this->setup = setup;
}

void cServerStats::pktSent(const u_int64_t conn_id, const timespec ts, const uint16_t len) {
    if (s_stats.count(conn_id) == 0) {
        stats = new s_stats_t;
        stats->tx_bitrare = 0;
        stats->rx_bitrare = 0;
        stats->src = "";
        stats->tx_pkts = 0;
        stats->rx_pkts = 0;
        s_stats[conn_id] = stats;
    }
    stats = s_stats[conn_id];
    stats->tx_pkts++;
    stats->tx_bitrare = this->pk_enque(conn_id, TX, ts, len);
}

void cServerStats::pktReceived(const u_int64_t conn_id, const timespec ts, const uint16_t len) {
    if (s_stats.count(conn_id) == 0) {
        stats = new s_stats_t;
        stats->tx_bitrare = 0;
        stats->rx_bitrare = 0;
        stats->src = "";
        stats->tx_pkts = 0;
        stats->rx_pkts = 0;
        s_stats[conn_id] = stats;
    }
    stats = s_stats[conn_id];
    stats->rx_pkts++;
    stats->rx_bitrare = this->pk_enque(conn_id, RX, ts, len);
}

void cServerStats::printRealTime(void) const {
    std::stringstream ss;
    timespec curTv;

    clock_gettime(CLOCK_REALTIME, &curTv);
    ss.str("");
    ss << "cServerStats:\n";

    ss << "ts: " << curTv.tv_sec << ".";
    ss.fill('0');
    ss.width(9);
    ss << curTv.tv_nsec;
    ss << "\n";
    ss.precision(0);

    for (std::map<u_int64_t, s_stats_t *>::const_iterator it = s_stats.begin(); it != s_stats.end(); ++it) {
        ss << "\nClient >>>: " << it->second->src << std::endl;
        ss << "tx_packets: " << it->second->tx_pkts << std::endl;
        ss << "rx_packets: " << it->second->rx_pkts << std::endl;
        ss << "tx_bitrate: " << it->second->tx_bitrare << std::endl;
        ss << "rx_bitrate: " << it->second->rx_bitrare << std::endl;
    }
    std::cout << ss.str() << endl;
}

void cServerStats::printSummary(void) const {

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

void cServerStats::setClientIP(const u_int64_t conn_id, const string src) {
    if (s_stats.count(conn_id) == 0) {
        stats = new s_stats_t;
        stats->tx_bitrare = 0;
        stats->rx_bitrare = 0;
        stats->src = src;
        stats->tx_pkts = 0;
        stats->rx_pkts = 0;
        s_stats[conn_id] = stats;
    } else {
        if (s_stats[conn_id]) {
            s_stats[conn_id]->src = src;
        }
    }
}