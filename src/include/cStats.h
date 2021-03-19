/*
 * File:   cStats.h
 *
 * Copyright (C) 2017: Department of Telecommunication Engineering, FEE, CTU in Prague
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

#ifndef CSTATS_H
#define CSTATS_H

#include "_types.h"
#include "cSetup.h"

//timestamp;hostname;test_duration;bytes_sent;bytes_received;avg_bitrate_tx, avg_bitrate_rx;avg_rtt;avg_pk_loss;current_bitrate_tx;current_bitrate_rx;current_rtt;current_pk_loss

using namespace std;


struct stats_t{
    u_int64_t rx_pkts;
    u_int32_t rx_pps;
    u_int64_t tx_bitrare;
    u_int64_t rx_bitrare;
    u_int64_t tx_bytes;
    u_int64_t rx_bytes;
    float rx_loss;
    float cur_loss;
    u_int64_t duration; //ms
    u_int64_t test_start; //ns
    u_int64_t ooo_pkts;
    u_int64_t max_seq;
};

struct c_stats_t : stats_t {
    u_int64_t tx_pkts;
    u_int32_t tx_pps;
    u_int64_t cumulative_rtt;
    float rtt_min;
    float rtt_max;
    float rtt_avg;
    float cur_rtt;
    std::string dst;
    u_int64_t server_rx_pkts;
};

struct s_stats_t : stats_t {
    std::string src;
    u_int32_t port;
};

struct pinfo_t {
    timespec ts;
    u_int16_t len;
    u_int64_t seq;
    float rtt;
};

struct qstats_t {
    u_int64_t tx_qsize; //Bits
    u_int64_t tx_qtime; //Q time len in ms
    u_int64_t rx_qsize; //Bits
    u_int64_t rx_qtime; //Q time len in ms
    u_int64_t rx_q_cumulative_rtt; //usec
    u_int64_t max_seq;
};

class cStats {
public:
    cStats();
    virtual ~cStats();
    virtual void printSummary(void);
    virtual void printRealTime(void);
    virtual void printStatus(void);
    virtual void setStatus(std::string status);
protected:
    //enqueue + queue management + stats update; 
    void pk_enque(const u_int64_t conn_id, const uint16_t direction, const timespec ts, const u_int16_t len, const u_int64_t seq, const float rtt);
    void prepareStats(const u_int64_t conn_id, const uint16_t direction, stats_t * stats);
    cSetup *setup;

private:
    map<u_int64_t, queue<pinfo_t> *> pk_info_rx_queues;
    map<u_int64_t, queue<pinfo_t> *> pk_info_tx_queues;
    map<u_int64_t, qstats_t *> qstats;
    std::string status;
};


class cServerStats : public cStats {
public:
    cServerStats(cSetup *setup);
    ~cServerStats();
    virtual void printSummary(void);
    virtual void printRealTime(void);
    void pktSent(const u_int64_t conn_id, const timespec ts, const uint16_t len, const u_int64_t seq, const std::string src, const u_int32_t port); //increment tx_pkts & calculate bitrate;
    void pktReceived(const u_int64_t conn_id, const timespec ts, const u_int16_t len, const u_int64_t seq, const std::string src, const u_int32_t port); //increment rx_pkts & calculate bitrate;
    u_int16_t connStatRemove(const u_int64_t conn_id);
    void connInit(const u_int64_t conn_id, const string src, const u_int32_t port);
private:
    map<u_int64_t, s_stats_t *> s_stats;
    s_stats_t *stats;
};

class cClientStats : public cStats {
public:
    cClientStats(cSetup *setup);
    ~cClientStats();
    std::string getReport(void);
    void pktOoo(void); //increment OutOfOrder packet counter;
    void pktSent(const timespec ts, const uint16_t len, const u_int64_t seq); //increment tx_pkts  & calculate bitrate;
    void pktRecv(const timespec ts, const u_int16_t len, const u_int64_t seq, const float rtt); //increment rx_pkts & calculate bitrate;
    void pktSent(const u_int64_t ts, const uint16_t len, const u_int64_t seq); //increment tx_pkts  & calculate bitrate;
    void pktRecv(const u_int64_t ts, const u_int16_t len, const u_int64_t seq, const float rtt); //increment rx_pkts & calculate bitrate;
    void addServerStats(const u_int64_t server_rx_pkts);
    virtual void printSummary(void);
    virtual void printRealTime(void);
private:
    c_stats_t *stats;
};
#endif /* CSTATS_H */

