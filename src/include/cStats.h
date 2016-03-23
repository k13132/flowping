/*
 * File:   cStats.h
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

#ifndef CSTATS_H
#define CSTATS_H

#include "_types.h"
#include "cSetup.h"

struct c_stats_t{
    u_int64_t tx_pkts;
    u_int64_t rx_pkts;
    u_int64_t server_rx_pkts;
    u_int64_t ooo_pkts;
    u_int32_t bitrare;
    double rtt_min;
    double rtt_max;
    double rtt_avg;
    std::string dst;
    u_int64_t duration;
};

struct s_stats_t{
    u_int64_t tx_pkts;
    u_int64_t rx_pkts;
    u_int32_t bitrare;
    std::string src;
};

class cStats {
public:
    cStats();
    virtual ~cStats();
    virtual void printSummary(void) const;
    virtual void printRealTime(void) const;
private:

};

class cServerStats:public cStats{
public:
    cServerStats(cSetup *setup);
    virtual void printSummary(void) const;
    virtual void printRealTime(void) const;
private:
    cSetup *setup;
    s_stats_t stats;
};

class cClientStats:public cStats{
public:
    cClientStats(cSetup *setup);
    std::string getReport(void) const;
    void pktSent(void); //increment tx_pkts;
    void pktOoo(void); //increment OutOfOrder packet counter;
    void addRTT(const double rtt);
    void addServerStats(const u_int64_t server_rx_pkts);
    virtual void printSummary(void) const;
    virtual void printRealTime(void) const;
private:
    cSetup *setup;
    c_stats_t stats;
};
#endif /* CSTATS_H */

