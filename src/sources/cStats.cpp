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

cStats::cStats() {
    std::cout << "X\n";
}

cStats::~cStats() {
}

void cStats::printRealTime(void) const {
    std::cout << "cStats\n";
}

void cStats::printSummary(void) const {

}

cClientStats::cClientStats(cSetup *setup) {
    stats.bitrare=0;
    stats.dst="";
    stats.rtt_avg=0;
    stats.rtt_max=0;
    stats.rtt_min=INT_MAX;
    stats.rx_pkts=0;
    stats.tx_pkts=0;
    stats.ooo_pkts=0;
    stats.server_rx_pkts=0;
    this->setup=setup;
}

void cClientStats::printRealTime(void) const {
    std::cout << "cClientStats\n";
    std::cout << "avg_rtt: "<<stats.rtt_avg<<std::endl;
    std::cout << "rx_packets: "<<stats.rx_pkts<<std::endl;
}

void cClientStats::printSummary(void) const {

}

void cClientStats::addServerStats(const u_int64_t server_rx_pkts){
    stats.server_rx_pkts = server_rx_pkts;
}

std::string cClientStats::getReport() const{
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
    ss <<stats.rtt_min <<"/"<< stats.rtt_avg <<"/"<< stats.rtt_max <<" ms ";
    ss << "Out of Order = "<<stats.ooo_pkts<<" packets\n";

    ss << "\n---Server report--- "<< setup->getHostname() <<" ping statistics ---\n";
    ss << stats.server_rx_pkts<<" received, ";
    ss<< uloss<<"% upstream packet loss, ";
    ss << dloss<<"% downstream packet loss\n\n";
    return ss.str();
}

void cClientStats::addRTT(const double rtt) {
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
    
}

void cClientStats::pktSent(){
    stats.tx_pkts++;
}

void cClientStats::pktOoo(){
    stats.ooo_pkts++;
}

cServerStats::cServerStats(cSetup *setup) {
    stats.bitrare=0;
    stats.src="";
    stats.tx_pkts=0;
    stats.rx_pkts=0;
    setup=setup;
}


void cServerStats::printRealTime(void) const {
    std::cout << "cServerStats\n";
}

void cServerStats::printSummary(void) const {

}
