/* 
 * File:   cSetup.cpp
 * 
 * Author: Ondrej Vondrous
 * Email: vondrous@0xFF.cz
 *
 * Created on 26. červen 2012, 22:38
 */

#include "cSetup.h"
#include "cServer.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>

cSetup::cSetup(int argc, char **argv, string version) {
    this->version = "Not Defined";
    fp = stdout; //output to terminal
    this->a_par = false;
    this->A_par = false;
    this->v_par = false;
    this->p_par = false;
    this->d_par = false;
    this->I_par = false;
    this->i_par = false;
    this->t_par = false;
    this->T_par = false;
    this->b_par = false;
    this->B_par = false;
    this->q_par = false;
    this->f_par = false;
    this->h_par = false;
    this->F_par = false;
    this->c_par = false;
    this->D_par = false;
    this->w_par = false;
    this->n_par = false;
    this->s_par = false;
    this->S_par = false;
    this->X_par = false;
    this->P_par = false;
    this->R_par = false;
    this->r_par = false;
    this->Q_par = false;
    this->W_par = false;
    this->_par = false;
    this->port = 2424;
    this->interval_i = 1000000; // 1s
    this->interval_I = 1000000; // 1s
    this->time_t = 10; // 10s
    this->time_T = 10; // 10s
    this->size = 64; // 64B Payload
    this->rate_b = 1; // 1kbit/s
    this->rate_B = 1; // 1kbit/s
    this->step = 0; // 1bp/us
    this->deadline = 0; // 0s
    this->count = ULONG_MAX;
    this->filename = "";
    this->srcfile = "";
    this->host = "localhost";
    int c;
    this->brate = 0;
    this->erate = 0;
    this->bts = 0;
    this->ets = 0;
    this->tp_exhausted = false;

    this->version = version;
    while ((c = getopt(argc, argv, "XUeEaAQSqdDPH?w:p:c:h:s:i:nFf:u:vI:t:T:b:B:r:R:")) != EOF) {
        switch (c) {
            case 'v':
                this->v_par = true;
                break;
            case 'a':
                this->a_par = true;
                break;
            case 'A':
                this->A_par = true;
                break;
            case 'q':
                this->q_par = true;
                break;
            case 'Q':
                this->Q_par = true;
                break;
            case 'p':
                this->p_par = true;
                this->port = atoi(optarg);
                break;
            case 'P':
                this->P_par = true;
                break;
            case 'd':
                this->d_par = true;
                break;
            case 'I':
                this->I_par = true;
                this->interval_I = atof(optarg)*1000000;
                if (this->interval_I > MAX_INTERVAL) this->interval_I = MAX_INTERVAL;
                if (this->interval_I < MIN_INTERVAL) this->interval_I = MIN_INTERVAL;
                break;
            case 'i':
                this->i_par = true;
                this->interval_i = atof(optarg)*1000000;
                if (this->interval_i > MAX_INTERVAL) this->interval_i = MAX_INTERVAL;
                if (this->interval_i < MIN_INTERVAL) this->interval_i = MIN_INTERVAL;
                break;
            case 't': //T1
                this->t_par = true;
                this->time_t = atof(optarg);
                break;
            case 'T': //T2
                this->T_par = true;
                this->time_T = atof(optarg);
                break;
            case 'b':
                this->b_par = true;
                this->rate_b = atoi(optarg);
                break;
            case 'B':
                this->B_par = true;
                this->rate_B = atoi(optarg);
                break;
            case 'r': //STEP
                this->r_par = true;
                this->step = atoi(optarg);
                break;
            case 'R': //T3
                this->R_par = true;
                this->time_R = atof(optarg);
                break;
            case 'f':
                this->f_par = true;
                this->filename = optarg;
                break;
            case 'u':
                this->u_par = true;
                this->srcfile = optarg;
                if (this->parseSrcFile()) {
                    cout << "Error parsing time_dev file!" << endl;
                }
                break;
            case 'h':
                this->h_par = true;
                this->host = optarg;
                break;
            case 'H':
                this->H_par = true;
                break;
            case 'F':
                this->F_par = true;
                break;
            case 'X':
                this->X_par = true;
                break;
            case 'U':
                this->U_par = true;
                break;
            case 'e':
                this->e_par = true;
                break;
            case 'E':
                this->E_par = true;
                break;
            case 'S':
                this->S_par = true;
                break;
            case 'c':
                this->c_par = true;
                this->count = atol(optarg);
                break;
            case 'D':
                this->D_par = true;
                break;
            case 'w':
                this->w_par = true;
                this->deadline = atoi(optarg);
                break;
            case 'n':
                //nedala nic;
                break;
            case 's':
                this->s_par = true;
                if (atoi(optarg) > MAX_PKT_SIZE) {
                    this->size = MAX_PKT_SIZE;
                } else {
                    this->size = atoi(optarg);
                }
                break;
            case '?':
                this->_par = true;
                break;

            default:
                this->_par = true;
                break;
        }
    }
    if ((this->i_par) && (not this->I_par)) this->interval_I = this->interval_i;
    if ((this->I_par) && (not this->i_par)) this->interval_i = this->interval_I;
    if (this->b_par && not (this->i_par or this->I_par)) {
        this->interval_i = (((this->getPacketSize())*8.0)*1000000.0) / (this->rate_b * 1000.0);
        this->interval_I = (((this->getPacketSize())*8.0)*1000000.0) / (this->rate_B * 1000.0);
    }
    if (this->B_par && not (this->i_par or this->I_par)) {
        this->interval_i = (((this->getPacketSize())*8.0)*1000000.0) / (this->rate_b * 1000.0);
        this->interval_I = (((this->getPacketSize())*8.0)*1000000.0) / (this->rate_B * 1000.0);
    }
    if (this->P_par) this->size = MIN_PKT_SIZE;
}

cSetup::~cSetup() {
}

u_int8_t cSetup::self_check(void) {
    if (v_par) { //show version, then exit
        return SETUP_CHCK_VER;
    }
    if (_par) { //show usage
        return SETUP_CHCK_SHOW;
    }
    if (isServer()) {
        if (a_par || Q_par || b_par || B_par || c_par || F_par || h_par || H_par || i_par || I_par || s_par || t_par || T_par || R_par || w_par) {
            return SETUP_CHCK_ERR;
        }
    } else {
        if (X_par || S_par) {
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
    cout << endl << " Application:\e[1;37m uPing\e[0;37m (UDP based Ping)\e[0m" << endl;
    cout << " Version: \e[1;37m" << this->version << "\e[0m" << endl << endl;
}

void cSetup::usage() {
    cout << " -----------------------------------------------------------------------------------------------" << endl;
    cout << "| Section| Parameter  | Def. value | Meaning                                                    |" << endl;
    cout << " -----------------------------------------------------------------------------------------------" << endl;
    cout << "| Common:                                                                                       |" << endl;
    cout << "|         [-?]                       Usage (Print this table)                                   |" << endl;
    cout << "|         [-A]                       Raise priority to maximum in passive mode (RT if possible) |" << endl;
    cout << "|         [-d]                       Debug (more detailed output is generated to STDOUT)        |" << endl;
    cout << "|         [-D]                       Print timestamps in ping output                            |" << endl;
    cout << "|         [-e]                       Print current receving Bitrate                             |" << endl;
    cout << "|         [-f filename]              Store ping output in specified file                        |" << endl;
    cout << "|         [-p port]     [2424]       Port number                                                |" << endl;
    cout << "|         [-q]                       Silent (suppress ping output to STDOUT)                    |" << endl;
    cout << "|         [-v]                       Print version                                              |" << endl;
    cout << "| Server:                                                                                       |" << endl;
    cout << "|         [-S]                       Run server                                                 |" << endl;
    cout << "|         [-X]                       Asymetric mode (Payload in server reply is limited to 20B) |" << endl;
    cout << "| Client:                                                                                       |" << endl;
    cout << "|         [-a]                       Active waiting! (100% CPU usage), more accurate bitrate    |" << endl;
    cout << "|         [-b kbit/s]                BitRate (Lower limit)                                      |" << endl;
    cout << "|         [-B kbit/s]                BitRate (Upper limit)                                      |" << endl;
    cout << "|         [-c count]    [unlimited]  Send specified number of packets                           |" << endl;
    cout << "|         [-E]                       Print sending Bitrate                                      |" << endl;
    cout << "|         [-F filename]              Send FileName to server (prior to server settings)         |" << endl;
    cout << "|         [-h hostname] [localhost]  Server hostname or IP address                              |" << endl;
    cout << "|         [-H]                       Consider headers (Use frame size instead payload size)     |" << endl;
    cout << "|         [-i seconds]  [1]          Interval between packets (Upper limit)                     |" << endl;
    cout << "|         [-I seconds]  [1]          Interval between packets (Lower limit)                     |" << endl;
    cout << "|         [-P]                       Packet size change from 22B to 1472B                       |" << endl;
    cout << "|         [-Q]                       linux ping output compatibility                            |" << endl;
    cout << "|         [-s size]     [64]         Payload size in Bytes                                      |" << endl;
    cout << "|         [-t seconds]  [10]         T1 interval specification  (for i,I,b,B,P params)          |" << endl;
    cout << "|         [-T seconds]  [10]         T2 interval specification  (for i,I,b,B,P params)          |" << endl;
    cout << "|         [-R seconds]  [T3=T2]      T3 interval specification  (for i,I,b,B,P params)          |" << endl;
    cout << "|         [-u filename]              Read Interval and BitRate definitions from file           |" << endl;
    cout << "|         [-U]                       Fill packets with data from named pipe at /tmp/uping       |" << endl;
    cout << "|         [-w seconds]  [unlimited]  Run test for specified time                                |" << endl;
    cout << " -----------------------------------------------------------------------------------------------" << endl;
}

int cSetup::getPort() {
    return this->port;
}

bool cSetup::debug() {
    return this->d_par;
}

FILE * cSetup::getFP() {
    return this->fp;
}

bool cSetup::showTimeStamps() {
    return this->D_par;
}

bool cSetup::isAsym() {
    return this->X_par;
}

bool cSetup::sendFilename() {
    return this->F_par;
}

string cSetup::getFilename() {
    return this->filename;
}

string cSetup::getSrcFilename() {
    return this->srcfile;
}

bool cSetup::outToFile() {
    return this->f_par;
}

u_int32_t cSetup::getTime_t() {
    return (u_int32_t)this->time_t;
};

u_int32_t cSetup::getTime_T() {
    return (u_int32_t)this->time_T;

};

double cSetup::getTime_R() {
    return this->time_R;

};

u_int16_t cSetup::getPayloadSize() {
    return this->size;
}

string cSetup::getHostname() {
    return this->host;
}

u_int32_t cSetup::getDeadline() {
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
            if (T3 == 0) T3 = 0.0001;
        } else {
            T3 = this->getTime_T();
        }
        if (this->b_par && this->B_par) {
            bchange = (this->rate_B - this->rate_b)*1000 / (T3 * 1000000);
            return bchange;
        }
        if (this->i_par && this->I_par) {
            bchange = ((this->getPacketSize()*8 * 1000000) / this->interval_I - (this->getPacketSize()*8 * 1000000) / this->interval_i) / (T3 * 1000000);
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
        schange = 1450.0 / (T3 * 1000000);
        return schange;
    }
    return schange;
}

u_int64_t cSetup::getPacketSize() {
    if (this->H_par) {
        return this->size + 42;
    } else {
        return this->size;
    }
}

u_int64_t cSetup::getBaseRate() {
    if (this->b_par) {
        return this->rate_b * 1000;
    } else {
        return (u_int64_t) (this->getPacketSize()*8 * 1000000 / this->interval_i);
    }
}

bool cSetup::pkSizeChange() {
    return this->P_par;
}

bool cSetup::frameSize() {
    return this->H_par;
}

bool cSetup::compat() {
    return this->Q_par;
}

bool cSetup::showBitrate() {
    return this->e_par;
}

bool cSetup::showSendBitrate() {
    return this->E_par;
}

bool cSetup::speedUP() {
    if (u_par) return false;
    if (interval_I == interval_i) return true;
    if (i_par != I_par) {
        if (not(b_par || B_par)) return true;
    }
    if (b_par != B_par) {
        if (not(i_par || I_par)) return true;
    }
    return false;
}

bool cSetup::actWaiting() {
    return this->a_par;
}

bool cSetup::raisePriority() {
    return this->A_par;
}

bool cSetup::npipe() {
    return this->U_par;
}

bool cSetup::shape() {
    return this->u_par;
}

int cSetup::parseSrcFile() {
    if (this->getSrcFilename().length()) {
        ifstream infile;
        time_def tmp, check;
        check.ts = 0;
        infile.open(this->getSrcFilename().c_str());
        while (infile >> tmp.ts >> tmp.bitrate >> tmp.len) {
            if (tmp.len < MIN_PKT_SIZE) tmp.len = MIN_PKT_SIZE;
            if (tmp.len > MAX_PKT_SIZE) tmp.len = MAX_PKT_SIZE;
            r_tpoints.push_back(tmp);
        }
        while (r_tpoints.size() != 0) {
            tmp = r_tpoints.back();
            r_tpoints.pop_back();
            tpoints.push_back(tmp);
            if (tmp.ts < check.ts) {
                cerr << "Time inconsistency in source file!" << endl;
            }
            //cout << tpoints.size() << endl;
        }
        infile.close();
    } else {
        return 1;
    }
    return 0;
}

//ts - RT from uping start in usecs.

double cSetup::getRTBitrate(u_int64_t ts) {
    if (!tp_exhausted) {
        if (ts > (this->ets * 1000000)) {
            this->bts = this->ets;
            this->brate = this->erate;
            if (tpoints.size()) {
                td_tmp = (time_def) tpoints.back();
                ets = td_tmp.ts;
                erate = td_tmp.bitrate;
                setPayoadSize(td_tmp.len);
                cerr<<ets<<"\t"<<erate<<"\t"<<td_tmp.len<<endl;
                tpoints.pop_back();
                //bitrate change / usec
                if ((ets - bts) != 0) {
                    double tmp1 = (double) (erate - brate);
                    double tmp2 = (double) ((ets - bts)*1000000.0);
                    //cout << "tmp1: "<<tmp1<<"\t tmp2: "<<tmp2<<endl;
                    bchange = double(tmp1 / tmp2);

                } else {
                    bchange = (double) (erate - brate);
                }
            } else {
                tp_exhausted = true;
            }
            //cout << "bts: " << bts << "\t ets: " << ets << "\t brate: " << brate << "\t erate: " << erate << "\t bchange:" << bchange << endl;
        }
    }
    //cout << "bitrate: " << (brate * 1000 + bchange * 1000 * (ts-bts*1000000)) << endl;

    return (brate * 1000 + bchange * 1000 * (ts - bts * 1000000));
}

void cSetup::setPayoadSize(u_int16_t psize){
    this->size=psize;
}