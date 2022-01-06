/*
 * File:   cSetup.cpp
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
 *
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



#include "_types.h"
#include "cSetup.h"
#include <cmath>


using namespace std;

cSetup::cSetup(int argc, char **argv, string version) {
    this->sample_len = 0;
    this->ipv6 = false;
    this->tp_ready = false;
    this->td_tmp.len = 0;
    this->td_tmp.bitrate = 0;
    this->td_tmp.ts = 0;
    this->version = "Not Defined";
    output = &std::cout; //output to terminal
    this->done = false;
    this->vonly = true;
    this->a_par = false;
    this->v_par = false;
    this->p_par = false;
    this->d_par = false;
    this->I_par = false;
    this->i_par = false;
    this->L_par = false;
    this->t_par = false;
    this->T_par = false;
    this->b_par = false;
    this->B_par = false;
    this->q_par = false;
    this->f_par = false;
    this->h_par = false;
    this->H_par = false;
    this->u_par = false;
    this->F_par = false;
    this->c_par = false;
    this->e_par = false;
    this->E_par = false;
    this->C_par = false;
    this->D_par = false;
    this->w_par = false;
    this->s_par = false;
    this->S_par = false;
    this->X_par = false;
    this->XR_par = false;
    this->R_par = false;
    this->r_par = false;
    this->W_par = false;
    this->J_par = false;
    this->_par = false;
    this->antiAsym = false;
    this->port = 2424;
    this->interval_i = 1000000000; // 1s
    this->interval_I = 1000000000; // 1s
    this->time_t = 0; // 0s
    this->time_T = 10; // 10s
    this->time_R = 10; // 10s
    this->size = 64; // 64B Payload
    this->rate_b = 1; // 1kbit/s
    this->rate_B = 1; // 1kbit/s
    this->step = 0; // 1bp/us
    this->deadline = 0; // 0s
    this->count = ULONG_MAX;
    this->filename = "";
    this->F_filename = "";
    this->srcfile = "";
    this->host = "localhost";
    int c;
    this->brate = 0;
    this->erate = 0;
    this->bts = 0;
    this->ets = 0;
    this->fpsize = 64;
    this->fpsize_set = false;

    this->version = version;
    while ((c = getopt(argc, argv, "JCXeEaSqDH6?w:d:p:c:h:s:i:F:f:u:vI:t:T:b:B:r:R:L:")) != EOF) {
        switch (c) {
            case 'v':
                this->v_par = true;
                break;
            case 'a':
                this->a_par = true;
                this->vonly = false;
                break;
            case 'C':
                this->C_par = true;
                this->vonly = false;
                break;
            case 'J':
                this->J_par = true;
                this->vonly = false;
                break;
            case 'q':
                this->q_par = true;
                this->vonly = false;
                break;
            case 'p':
                this->p_par = true;
                this->vonly = false;
                this->port = atoi(optarg);
                break;
            case 'd':
                this->d_par = true;
                this->vonly = false;
                this->interface = optarg;
                break;
            case 'I':
                this->vonly = false;
                this->I_par = true;
                this->interval_I = atof(optarg)*1000000000;
                if (this->interval_I > MAX_INTERVAL) this->interval_I = MAX_INTERVAL;
                if (this->interval_I < MIN_INTERVAL) this->interval_I = MIN_INTERVAL;
                break;
            case 'i':
                this->vonly = false;
                this->i_par = true;
                this->interval_i = atof(optarg)*1000000000;
                if (this->interval_i > MAX_INTERVAL) this->interval_i = MAX_INTERVAL;
                if (this->interval_i < MIN_INTERVAL) this->interval_i = MIN_INTERVAL;
                break;
            case 'L':
                this->L_par = true;
                this->vonly = false;
                this->sample_len = (u_int64_t)(atof(optarg)*1000000000); //in ns
                break;
            case 't': //T1
                this->vonly = false;
                this->t_par = true;
                this->time_t = atof(optarg);
                break;
            case 'T': //T2
                this->vonly = false;
                this->T_par = true;
                this->time_T = atof(optarg);
                if (!this->R_par){
                    this->time_R = this->time_T;
                }
                break;
            case 'b':
                this->vonly = false;
                this->b_par = true;
                this->rate_b = atoi(optarg);
                break;
            case 'B':
                this->vonly = false;
                this->B_par = true;
                this->rate_B = atoi(optarg);
                break;
            case 'r': //STEP
                this->vonly = false;
                this->r_par = true;
                this->step = atoi(optarg);
                break;
            case 'R': //T3
                this->vonly = false;
                this->R_par = true;
                this->time_R = atof(optarg);
                if (!this->T_par){
                    this->time_T = this->time_R;
                }
                break;
            case 'f':
                this->vonly = false;
                this->f_par = true;
                this->filename = optarg;
                break;
            case 'u':
                this->vonly = false;
                this->u_par = true;
                this->srcfile = optarg;
                break;
            case 'h':
                this->vonly = false;
                this->h_par = true;
                this->host = optarg;
                break;
            case 'H':
                this->vonly = false;
                this->H_par = true;
                break;
            case 'F':
                this->vonly = false;
                this->F_par = true;
                this->F_filename = optarg;
                break;
            case 'X':
                this->vonly = false;
                this->X_par = true;
                this->XR_par = true;
                this->antiAsym = true;
                break;
            case 'e':
                this->vonly = false;
                this->e_par = true;
                break;
            case 'E':
                this->vonly = false;
                this->E_par = true;
                break;
            case 'S':
                this->vonly = false;
                this->S_par = true;
                break;
            case 'c':
                this->vonly = false;
                this->c_par = true;
                this->count = atol(optarg);
                break;
            case 'D':
                this->vonly = false;
                this->D_par = true;
                break;
            case 'W':
                this->vonly = false;
                this->W_par = true;
                break;
            case 'w':
                this->vonly = false;
                this->w_par = true;
                this->deadline = atoi(optarg);
                break;
            case '6':
                this->vonly = false;
                this->ipv6 = true;
                break;
            case 's':
                this->s_par = true;
                this->vonly = false;
                if (atoi(optarg) > MAX_PKT_SIZE) {
                    this->size = MAX_PKT_SIZE;
                } else {
                    this->size = atoi(optarg);
                }
                if (this->size < MIN_PKT_SIZE){
                    cerr << "Wrong packet length!" << endl;
                    exit(1);
                }
                if (!fpsize_set) {
                    fpsize = this->size;
                    fpsize_set = true;
                }
                break;
            case '?':
                this->_par = true;
                this->vonly = false;
                break;

            default:
                this->_par = true;
                break;
        }
    }
    if ((this->_par) || (this->vonly)) return;
    if ((this->i_par) && (not this->I_par)) this->interval_I = this->interval_i;
    if ((this->I_par) && (not this->i_par)) this->interval_i = this->interval_I;
    if (this->b_par && not (this->i_par or this->I_par)) {
        this->interval_i = (((this->getPacketSize())*8.0)*1000000000.0) / (this->rate_b * 1000.0);
        this->interval_I = (((this->getPacketSize())*8.0)*1000000000.0) / (this->rate_B * 1000.0);
    }
    if (this->B_par && not (this->i_par or this->I_par)) {
        this->interval_i = (((this->getPacketSize())*8.0)*1000000000.0) / (this->rate_b * 1000.0);
        this->interval_I = (((this->getPacketSize())*8.0)*1000000000.0) / (this->rate_B * 1000.0);
    }
    if (H_par) {
        //minimu size of FP frame;
        if (this->size < HEADER_LENGTH + 42) {
            this->size = HEADER_LENGTH + 42;
        }
    }
    if (u_par) {
        if (this->parseSrcFile()) {
            cerr << "Error parsing time_def file!" << endl;
            exit(1);
        }
    } else {
        if (this->parseCmdLine()) {
            cerr << "Error parsing cmdline parameters!" << endl;
            exit(1);
        }
    }
}

cSetup::~cSetup() {

}

u_int8_t cSetup::self_check(void) {
    if (C_par && J_par){
            return SETUP_CHCK_ERR;
    }
    if (_par) { //show usage
        return SETUP_CHCK_SHOW;
    }
    if (v_par) { //show version
        return SETUP_CHCK_VER;
    }
    if (isServer()) {
        if (a_par || b_par || B_par || c_par || F_par || J_par || h_par || H_par || i_par || I_par || s_par || t_par || T_par || R_par || w_par) {
            return SETUP_CHCK_ERR;
        }
    } else {
        //ToDo Remove in other than F-Tester edition of FlowPing
        if (not J_par) {
            return SETUP_CHCK_ERR;
        }
    }
    return SETUP_CHCK_OK;
}

u_int16_t cSetup::getMaxPacketSize(void) {
    return MAX_PKT_SIZE;
}

bool cSetup::isServer() {
    return this->S_par;
}

bool cSetup::isClient() {
    return not this->S_par;
}

void cSetup::show_version() {
    cout << endl << "Application:\e[1;37m FlowPing\e[0;37m (UDP based Ping)\e[0m" << endl;
    cout << "Version: \e[1;37m" << this->version << "\e[0m" << endl << endl;
}

string cSetup::get_version() {
    stringstream ss;
    ss.str("");
    ss << "FlowPing " << this->version << endl << endl;
    return ss.str();
}

string cSetup::getVersion() {
    stringstream ss;
    ss.str("");
    ss << "FlowPing " << this->version;
    return ss.str();
}

void cSetup::setAddrFamily(sa_family_t family){
    if ((family == AF_INET)||(family == AF_INET6)) this->addr_family = family;
}

sa_family_t cSetup::getAddrFamily(void){
    return this->addr_family;
}

void cSetup::usage() {
    cout << " -----------------------------------------------------------------------------------------------" << endl;
    cout << "| Section| Parameter  | Def. value | Meaning                                                    |" << endl;
    cout << " -----------------------------------------------------------------------------------------------" << endl;
    cout << "| Common:                                                                                       |" << endl;
    cout << "|         [-?]                       Usage (Print this table)                                   |" << endl;
    cout << "|         [-D]                       Print timestamps in ping output                            |" << endl;
    cout << "|         [-e]                       Print current RX Bitrate                                   |" << endl;
    cout << "|         [-E]                       Print current TX Bitrate                                   |" << endl;
    cout << "|         [-f filename]              Store ping output in specified file                        |" << endl;
    cout << "|         [-p port]     [2424]       Port number                                                |" << endl;
    cout << "|         [-q]                       Silent (suppress ping output)                              |" << endl;
    cout << "|         [-v]                       Print version                                              |" << endl;
    cout << "|         [-X]                       Asymmetric mode (TX Payload  is limited to 32 B)           |" << endl;
    cout << "| Server:                                                                                       |" << endl;
    cout << "|         [-S]                       Run as server                                              |" << endl;
    cout << "| Client:                                                                                       |" << endl;
    cout << "|         [-6]                       Prefer IPv6 over IPv4                                      |" << endl;
    cout << "|         [-a]                       Busy-loop mode! (100% CPU usage), more accurate bitrate    |" << endl;
    cout << "|         [-b kbit/s]                BitRate (first limit)                                      |" << endl;
    cout << "|         [-B kbit/s]                BitRate (second limit)                                     |" << endl;
    cout << "|         [-c count]    [unlimited]  Send specified number of packets                           |" << endl;
    cout << "|         [-C]                       CSV output format [;;;;]                                   |" << endl;
    cout << "|         [-J]                       JSON output format                                         |" << endl;
    cout << "|         [-d]                       Set source interface                                       |" << endl;
    cout << "|         [-F filename]              Send FileName to server (overide server settings)          |" << endl;
    cout << "|         [-h hostname] [localhost]  Server hostname or IP address                              |" << endl;
    cout << "|         [-i seconds]  [1]          Interval between packets (first limit)                     |" << endl;
    cout << "|         [-I seconds]  [1]          Interval between packets (second limit)                    |" << endl;
    cout << "|         [-L seconds]  [0]          Data sample interval (0 s - per packet mode)               |" << endl;
    cout << "|         [-s size]     [64]         Payload size in Bytes                                      |" << endl;
    cout << "|         [-t seconds]  [0]          T1 interval specification  (for i,I,b,B params)            |" << endl;
    cout << "|         [-T seconds]  [T2=T3]      T2 interval specification  (for i,I,b,B params)            |" << endl;
    cout << "|         [-R seconds]  [T3=T2]      T3 interval specification  (for i,I,b,B params)            |" << endl;
    cout << "|         [-u filename]              Read Interval and BitRate definitions from file            |" << endl;
    cout << "|         [-w seconds]  [unlimited]  Run test for specified time                                |" << endl;
    cout << " -----------------------------------------------------------------------------------------------" << endl;
}

int cSetup::getPort() {
    return this->port;
}

bool cSetup::useInterface() {
    return this->d_par;
}

FILE * cSetup::getFP() {
    return this->fp;
}
std::ostream* cSetup::getOutput() {
    return this->output;
}

bool cSetup::showTimeStamps() {
    return this->D_par;
}

bool cSetup::showTimeStamps(bool val) {
    return val;
}

bool cSetup::isAsym() {
    return this->X_par;
}

bool cSetup::isAsym(bool val) {
    return val;
}

bool cSetup::sendFilename() {
    return this->F_par;
}

string cSetup::getFilename() {
    return this->filename;
}

string cSetup::getF_Filename() {
    if (not this->F_filename.empty()) return this->F_filename;
    else return this->filename;
}

string cSetup::getSrcFilename() {
    return this->srcfile;
}

bool cSetup::outToFile() {
    return this->f_par;
}

u_int64_t cSetup::getTime_t() {
    return (u_int64_t)this->time_t;
};

u_int64_t cSetup::getTime_T() {
    return (u_int64_t)this->time_T;

};

double cSetup::getTime_R() {
    return this->time_R;

};

string cSetup::getHostname() {
    return this->host;
}

u_int64_t cSetup::getDeadline() {
    return this->deadline;
}

u_int64_t cSetup::getCount() {
    return this->count;
}

double cSetup::getInterval() {
    return this->interval_i;
}

double cSetup::getInterval_i() {
    return this->interval_i;
}

double cSetup::getInterval_I() {
    return this->interval_I;
}

bool cSetup::silent() {
    return this->q_par;
}

u_int64_t cSetup::getMinInterval() {
    if (this->interval_i>this->interval_I) {
        return (u_int64_t)this->interval_I;
    } else {
        return (u_int64_t)this->interval_i;
    }
}

u_int64_t cSetup::getMaxInterval() {
    if (this->interval_i>this->interval_I) {
        return (u_int64_t)this->interval_i;
    } else {
        return (u_int64_t)this->interval_I;
    }
}

double cSetup::getBchange() {
    double bchange = 0;
    double T3;
    if (this->t_par && this->T_par) {
        if (this->R_par) {
            T3 = this->getTime_R();
            if (T3 == 0) T3 = 0.000001;
        } else {
            T3 = this->getTime_T();
        }
        if (this->b_par && this->B_par) {
            bchange = (this->rate_B - this->rate_b)*1000 / (T3 * 1000000000);
            return bchange;
        }
        if (this->i_par && this->I_par) {
            bchange = ((this->getPacketSize()*8 * 1000000000) / this->interval_I - (this->getPacketSize()*8 * 1000000000) / this->interval_i) / (T3 * 1000000000);
            return bchange;
        }
    }
    return bchange;
}

double cSetup::getSchange() {
    double schange = 0;
    double T3;
    if (this->t_par && this->T_par) {
        if (this->R_par) {
            T3 = this->getTime_R();
        } else {
            T3 = this->getTime_T();
        }
        schange = 1476.0 / (T3 * 1000000000);
        return schange;
    }
    return schange;
}

u_int16_t cSetup::getPacketSize() {
    return this->size;
}

u_int64_t cSetup::getBaseRate() {
    if (this->b_par) {
        return this->rate_b * 1000;
    } else {
        return (u_int64_t) (this->getPacketSize()*8 * 1000000000 / this->interval_i);
    }
}
bool cSetup::frameSize() {
    return this->H_par;
}

bool cSetup::showBitrate() {
    return this->e_par;
}

bool cSetup::showSendBitrate() {
    return this->E_par;
}

bool cSetup::actWaiting() {
    return this->a_par;
}

bool cSetup::descFileInUse() {
    return this->u_par;
}

timed_packet_t cSetup::get_tmp_tpck(){
    return tmp_tpck;
}

timespec cSetup::getLastDelay() {
    return last_delay;
}

void cSetup::recordLastDelay(timespec last_delay) {
    this->last_delay = last_delay;
}

std::ostream& operator<<(std::ostream& os, const tpoint_def_t& obj)
{
  os << "ts:" << obj.ts << " rate:" << obj.bitrate << " length:"<<obj.len;
  return os;
}


int cSetup::parseCmdLine() {
    //Overit zda to funguje - pripadne zda to takto budeme delat?
    //cout << this->size << "\t" << this->interval_i << "\t" << this->interval_I << endl;
    u_int32_t shift = 0;
    tpoint_def_t tmp;
    tmp.ts = 0;
    if (deadline == 0) {
        deadline = 31536000; //~ 1 year
    }
    while(shift < this->deadline){
        if (t_par) {
            tmp.bitrate = 8000000.0 * this->size / this->interval_i;
            tmp.len = this->size;
            tpoints.push(tmp);
            tmp.ts = this->time_t + shift;
            tpoints.push(tmp);
            //cout << tmp.ts << "\t" << tmp.bitrate << "\t" << tmp.len << endl;
        } else {
            this->time_t = 0;
            tmp.ts = shift;
            tmp.bitrate = 8000000.0 * this->size / this->interval_i;
            tmp.len = this->size;
            tpoints.push(tmp);
        }
        if (R_par) {
            tmp.ts = this->time_t + this->time_R + shift;
            tmp.bitrate = 8000000.0 * this->size / this->interval_I;
            tmp.len = this->size;
            tpoints.push(tmp);
        }
        if (T_par) {
            tmp.ts = this->time_t + this->time_T + shift;
            tmp.bitrate = 8000000.0 * this->size / this->interval_I;
            tmp.len = this->size;
            tpoints.push(tmp);
        }
        if (!R_par && !T_par && !t_par) {
            tmp.ts = 86400;
            //ODcout << tmp.ts << endl;
            tmp.bitrate = 8000000.0 * this->size / this->interval_i;
            tmp.len = this->size;
            tpoints.push(tmp);
        }
        shift += this->time_t + this->time_T;
        if (this->time_t + this->time_T == 0){
            shift += 86400;
        }
    }
    //last record expected to be doubled;
    if (deadline == 0) {
        deadline = tmp.ts;
    }
    tpoints.push(tmp);
    refactorTPoints();
    this->tp_ready = true;
    return 0;
}

int cSetup::parseSrcFile() {
    bool setsize = true;
    string stmp;
    char * str;
    char * xstr;
    if (this->getSrcFilename().length()) {
        ifstream infile;
        tpoint_def_t tmp, check;
        tmp.bitrate = 0;
        tmp.len = 0;
        tmp.ts = 0;
        check.bitrate = 0;
        check.len = 0;
        check.ts = 0;
        infile.open(this->getSrcFilename().c_str());
        if (!infile.is_open()) {
            cerr << "Can't open source file! \"" << this->getSrcFilename() << "\"" << endl;
            exit(1);
        }
        while (getline(infile, stmp)) {
            str = strdup(stmp.c_str());
            //std::cout << strlen(str) <<std::endl;
            if (strlen(str)==0){
                continue;
            }
            xstr = strtok(str, "\t ,;");
            if (xstr == NULL) {
                cerr << "Can't parse source file!" << endl;
                exit(1);
            }
            tmp.ts = atof(xstr);
            xstr = strtok(NULL, "\t ,;");
            if (xstr == NULL) {
                cerr << "Can't parse source file!" << endl;
                exit(1);
            }
            tmp.bitrate = atoi(xstr);
            xstr = strtok(NULL, "\t ,;");
            if (xstr == NULL) {
                cerr << "Empty line!" << endl;
                //exit(1);
            }
            if (!fpsize_set) {
                fpsize = atoi(xstr);
                if (fpsize < MIN_PKT_SIZE) fpsize = MIN_PKT_SIZE;
                if (fpsize > MAX_PKT_SIZE) fpsize = MAX_PKT_SIZE;
                fpsize_set = true;
            }
            tmp.len = atoi(xstr);
            if (this->frameSize()) {
                tmp.len -= 42; //todo check negative size of payload.
            }
            if (tmp.len < MIN_PKT_SIZE) tmp.len = MIN_PKT_SIZE;
            if (tmp.len > MAX_PKT_SIZE) tmp.len = MAX_PKT_SIZE;
            if (tmp.ts < check.ts) {
                cerr << "Time inconsistency in source file!" << endl;
            }
            tpoints.push(tmp);
            if (setsize) {
                this->size = tmp.len;
                this->esize = this->size;
                //cerr << this->esize<<endl;
                setsize = false;
            }
        }
        tpoints.push(tmp);
        infile.close();
    } else {
        return 1;
    }
    if (deadline == 0) {
        deadline = ((tpoint_def_t) tpoints.back()).ts;
    }
    refactorTPoints();
    this->tp_ready = true;
    return 0;
}

void cSetup::refactorTPoints() {
    double reftime = 0;
    double last_ts = 0;
    if (tpoints.empty()) return;
    //???
    //if (((tpoint_def_t) tpoints.back()).ts > deadline) return;
    vector<tpoint_def_t> tmp_tpoints;
    while (!tpoints.empty()) {
        tmp_tpoints.push_back(tpoints.front());
        //cout << tpoints.front() << endl;
        tpoints.pop();
    }
    unsigned int index = 0;
    tpoint_def_t tp;
    while (last_ts + reftime < deadline) {
        if ((index > 0) && (index % (tmp_tpoints.size() - 1) == 0)) {
            reftime = reftime + tmp_tpoints[index].ts;
            index = 0;
            last_ts = 0;
        }
        tp = tmp_tpoints[index];
        last_ts = tp.ts;
        tp.ts = tp.ts + reftime;
        tpoints.push(tp);
        index++;
    }
}

bool cSetup::tpReady() {
    return this->tp_ready;
}

void cSetup::setPayoadSize(u_int16_t psize) {
    this->size = psize;
}

bool cSetup::is_vonly() {
    return this->vonly;
}

bool cSetup::wholeFrame() {
    return this->H_par;
}

bool cSetup::wholeFrame(bool val) {
    return val;
}

void cSetup::setHPAR(bool value) {
    this->H_par = value;
}

void cSetup::setWPAR(bool value) {
    this->W_par = value;
}

u_int64_t cSetup::getNextPacketTS(u_int64_t ts, u_int64_t sts, u_int64_t ets, u_int64_t srate, u_int64_t erate, u_int16_t len) {
    //std::cout << ts << ", " << sts << ", "<< ets << ", " << srate << ", " << erate << ", " << len << std::endl;
    if ((srate == 0)&&(erate == 0)) {
        return ets;
    }
    delta_rate = erate - srate;
    if (delta_rate == 0) {
        delay = (u_int64_t)8*1000000000 * len / erate;
    } else {
        nsec_delta = ts - sts;
        tp_diff = (ets - sts);
        if (nsec_delta == 0){
            //first packet delay
            if (srate == 0){
                delay = (u_int64_t) 1000000000 * std::sqrt( 16 * len / ((std::abs(delta_rate * 1000000000)) /(double)(tp_diff)));
            }else{
                delay = (u_int64_t) 8*1000000000 * len / srate;
            }
        }else{
            delay = (u_int64_t) 8*1000000000 * (len / (srate + (delta_rate *  (nsec_delta / (double)tp_diff)))); //interval [tv_nsec];
        }
    }
    return ts + delay;
}


bool cSetup::prepNextPacket() {
    //prepare and store packet in buffer
    if (!tpoints.empty()) {
        //run only once at beggining
        if (td_tmp.len == 0) {
            td_tmp = (tpoint_def_t) tpoints.front();
            s_tmp_rate = td_tmp.bitrate * 1000;
            s_tmp_ts = (u_int64_t) td_tmp.ts * 1000000000L;
            tmp_len = td_tmp.len;
            tmp_ts = s_tmp_ts;
            tpoints.pop();
        }
        td_tmp = (tpoint_def_t) tpoints.front();
        e_tmp_ts = (u_int64_t) td_tmp.ts * 1000000000L;
        e_tmp_rate = td_tmp.bitrate * 1000;
        if (e_tmp_ts != s_tmp_ts) {
            if (tmp_ts < e_tmp_ts) {
                tmp_ts = this->getNextPacketTS(tmp_ts, s_tmp_ts, e_tmp_ts, s_tmp_rate, e_tmp_rate, tmp_len);
                if ((s_tmp_rate != 0) || (e_tmp_rate != 0)) {
                    if (tmp_ts < (this->deadline * 1000000000)) {
                        tpacket.ts = tmp_ts;
                        tpacket.len = tmp_len;
                        if (tmp_len < 32 || tmp_len > 1500) {
                            std::cerr << "Packet size mismatch!\t" << tmp_len << std::endl;
                        }
                        //pthread_mutex_lock(&mutex);
                        pbuffer.push(tpacket);
                        //pthread_mutex_unlock(&mutex);
                    } else {
                        return false;

                    }
                }
            } else {
                s_tmp_rate = e_tmp_rate;
                s_tmp_ts = e_tmp_ts;
                tmp_len = td_tmp.len;
                tpoints.pop();
            }
        } else {
            s_tmp_rate = e_tmp_rate;
            s_tmp_ts = e_tmp_ts;
            tmp_len = td_tmp.len;
            tpoints.pop();
        }
    }else{
        return false;
    }
    return true;
}

timed_packet_t cSetup::getNextPacket() {
    tmp_tpck = *pbuffer.front();
    pbuffer.pop();
    return tmp_tpck;
}

bool cSetup::nextPacket() {
    return not pbuffer.empty();
}

u_int64_t cSetup::getTimedBufferSize() {
    return pbuffer.size();
}

void cSetup::setExtFilename(string s) {
    this->extfilename = s;
}

string cSetup::getExtFilename() {
    return this->extfilename;
}

uint16_t cSetup::extFilenameLen() {
    return this->extfilename.length();
}

string cSetup::getInterface() {
    return this->interface;
}

bool cSetup::toCSV(void) {
    return C_par;
}

bool cSetup::toJSON(void) {
    return J_par;
}

void cSetup::setCPAR(bool val) {
    C_par = val;
}

void cSetup::setJPAR(bool val) {
    J_par = val;
}

void cSetup::setXPAR(bool val) {
    X_par = val;
}

void cSetup::restoreXPAR() {
    X_par = XR_par;
}

bool cSetup::isAntiAsym() {
    return this->antiAsym;
}

bool cSetup::isAntiAsym(bool val) {
    return val;
}

void cSetup::setAntiAsym(bool val) {
    this->antiAsym = val;
}

u_int16_t cSetup::getFirstPacketSize() {
    return fpsize;
}

bool cSetup::isStarted() const {
    return started;
}

bool cSetup::isStop() const {
    return stop;
}

bool cSetup::isDone() const {
    return done;
}

void cSetup::setStarted(bool started) {
    cSetup::started = started;
}

void cSetup::setStop(bool stop) {
    cSetup::stop = stop;
}

void cSetup::setDone(bool done) {
    cSetup::done = done;
}

u_int64_t cSetup::getSampleLen() const {
    return sample_len;
}

bool cSetup::isIPv6Prefered() const {
    return ipv6;
}
