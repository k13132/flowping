/* 
 * File:   cSetup.cpp
 * 
 * Author: Ondrej Vondrous, KTT, CVUT
 * Email: vondrond@fel.cvut.cz
 * Copyright: Department of Telecommunication Engineering, FEE, CTU in Prague 
 * License: Creative Commons 3.0 BY-NC-SA

 * This file is part of FlowPing.
 *
 *  FlowPing is free software: you can redistribute it and/or modify
 *  it under the terms of the Creative Commons BY-NC-SA License v 3.0.
 *
 *  FlowPing is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY.
 *
 *  You should have received a copy of the Creative Commons 3.0 BY-NC-SA License
 *  along with FlowPing.
 *  If not, see <http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode>. 
 *   
 */


#include "cSetup.h"
#include "cServer.h"
#include "_types.h"

cSetup::cSetup(int argc, char **argv, string version) {
    this->version = "Not Defined";
    fp = stdout; //output to terminal
    this->vonly = true;
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
    this->C_par = false;
    this->D_par = false;
    this->w_par = false;
    this->n_par = false;
    this->s_par = false;
    this->S_par = false;
    this->X_par = false;
    this->XR_par = false;
    this->P_par = false;
    this->R_par = false;
    this->r_par = false;
    this->Q_par = false;
    this->W_par = false;
    this->_par = false;
    this->antiAsym = false;
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
    this->first_brate = true;

    this->version = version;
    while ((c = getopt(argc, argv, "CXUWeEaAQSqDPH?w:d:p:c:h:s:i:nFf:u:vI:t:T:b:B:r:R:")) != EOF) {
        switch (c) {
            case 'v':
                this->v_par = true;
                break;
            case 'a':
                this->a_par = true;
                this->vonly = false;
                break;
            case 'A':
                this->A_par = true;
                this->vonly = false;
                break;
            case 'C':
                this->C_par = true;
                this->vonly = false;
                break;
            case 'q':
                this->q_par = true;
                this->vonly = false;
                break;
            case 'Q':
                this->Q_par = true;
                this->vonly = false;
                break;
            case 'p':
                this->p_par = true;
                this->vonly = false;
                this->port = atoi(optarg);
                break;
            case 'P':
                this->P_par = true;
                this->vonly = false;
                break;
            case 'd':
                this->d_par = true;
                this->vonly = false;
                this->interface = optarg;
                break;
            case 'I':
                this->vonly = false;
                this->I_par = true;
                this->interval_I = atof(optarg)*1000000;
                if (this->interval_I > MAX_INTERVAL) this->interval_I = MAX_INTERVAL;
                if (this->interval_I < MIN_INTERVAL) this->interval_I = MIN_INTERVAL;
                break;
            case 'i':
                this->vonly = false;
                this->i_par = true;
                this->interval_i = atof(optarg)*1000000;
                if (this->interval_i > MAX_INTERVAL) this->interval_i = MAX_INTERVAL;
                if (this->interval_i < MIN_INTERVAL) this->interval_i = MIN_INTERVAL;
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
                if (this->parseSrcFile()) {
                    cout << "Error parsing time_dev file!" << endl;
                }
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
                break;
            case 'X':
                this->vonly = false;
                this->X_par = true;
                this->XR_par = true;
                break;
            case 'U':
                this->vonly = false;
                this->U_par = true;
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
            case 'n':
                this->vonly = false;
                //nedala nic;
                break;
            case 's':
                this->s_par = true;
                this->vonly = false;
                if (atoi(optarg) > MAX_PKT_SIZE) {
                    this->size = MAX_PKT_SIZE;
                } else {
                    this->size = atoi(optarg);
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
        if (S_par) {
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

void cSetup::usage() {
    cout << " -----------------------------------------------------------------------------------------------" << endl;
    cout << "| Section| Parameter  | Def. value | Meaning                                                    |" << endl;
    cout << " -----------------------------------------------------------------------------------------------" << endl;
    cout << "| Common:                                                                                       |" << endl;
    cout << "|         [-?]                       Usage (Print this table)                                   |" << endl;
    cout << "|         [-A]                       Raise priority to maximum in passive mode (RT if possible) |" << endl;
    cout << "|         [-D]                       Print timestamps in ping output                            |" << endl;
    cout << "|         [-e]                       Print current receving Bitrate                             |" << endl;
    cout << "|         [-f filename]              Store ping output in specified file                        |" << endl;
    cout << "|         [-p port]     [2424]       Port number                                                |" << endl;
    cout << "|         [-q]                       Silent (suppress ping output to STDOUT)                    |" << endl;
    cout << "|         [-v]                       Print version                                              |" << endl;
    cout << "|         [-X]                       Asymetric mode (Payload in reply is limited to 28B)        |" << endl;
    cout << "| Server:                                                                                       |" << endl;
    cout << "|         [-S]                       Run server                                                 |" << endl;
    cout << "| Client:                                                                                       |" << endl;
    cout << "|         [-a]                       Busy-waiting mode! (100% CPU usage), more accurate bitrate |" << endl;
    cout << "|         [-b kbit/s]                BitRate (Lower limit)                                      |" << endl;
    cout << "|         [-B kbit/s]                BitRate (Upper limit)                                      |" << endl;
    cout << "|         [-c count]    [unlimited]  Send specified number of packets                           |" << endl;
    cout << "|         [-C ]                      Output to CSV [;;;;]                                       |" << endl;
    cout << "|         [-d]                       Set source interface                                       |" << endl;
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
    cout << "|         [-u filename]              Read Interval and BitRate definitions from file            |" << endl;
    cout << "|         [-U]                       Fill packets with data from named pipe at /tmp/uping       |" << endl;
    cout << "|         [-w seconds]  [unlimited]  Run test for specified time                                |" << endl;
    cout << "|         [-W]                       Precompute packet intervals, Busy-waiting mode             |" << endl;
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

//int cSetup::parseSrcFile() {
//    bool setsize = true;
//    if (this->getSrcFilename().length()) {
//        ifstream infile;
//        tpoint_def_t tmp, check;
//        check.ts = 0;
//        infile.open(this->getSrcFilename().c_str());
//        while (infile >> tmp.ts >> tmp.bitrate >> tmp.len) {
//            if (tmp.len < MIN_PKT_SIZE) tmp.len = MIN_PKT_SIZE;
//            if (tmp.len > MAX_PKT_SIZE) tmp.len = MAX_PKT_SIZE;
//            if (tmp.ts < check.ts) {
//                cerr << "Time inconsistency in source file!" << endl;
//            }
//            tpoints.push(tmp);
//            if (setsize) {
//                this->size = tmp.len;
//                this->esize = this->size;
//                //cerr << this->esize<<endl;
//                setsize=false;
//            }
//        }
//        infile.close();
//    } else {
//        return 1;
//    }
//    return 0;
//}

int cSetup::parseSrcFile() {
    bool setsize = true;
    string stmp;
    char * str;
    char * xstr;
    if (this->getSrcFilename().length()) {
        ifstream infile;
        tpoint_def_t tmp, check;
        check.ts = 0;
        infile.open(this->getSrcFilename().c_str());
        if (!infile.is_open()){
                cerr << "Can't open source file! \""<< this->getSrcFilename()<<"\"" << endl;
                exit(1);
        }
        while (getline(infile, stmp)) {
            str = strdup(stmp.c_str());
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
                cerr << "Can't parse source file!" << endl;
                exit(1);
            }
            tmp.len = atoi(xstr);
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
            this->bsize = this->esize;
            if (tpoints.size()) {
                td_tmp = (tpoint_def_t) tpoints.front();
                ets = td_tmp.ts;
                erate = td_tmp.bitrate;
                esize = td_tmp.len;
                if (this->first_brate) {
                    this->first_brate = false;
                    this->brate = this->erate;
                }
                setPayoadSize(this->bsize);
                //cerr << ets << "\t" << erate << "\t" << td_tmp.len << endl;
                tpoints.pop();
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
    //cout << "X " << brate << "\t" << bchange << "\t" << ts << "\t" << bts << endl;
    return (brate * 1000 + bchange * 1000 * (ts - bts * 1000000));
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

void cSetup::setHPAR(bool value) {
    this->H_par = value;
}

void cSetup::setWPAR(bool value) {
    this->W_par = value;
}

bool cSetup::useTimedBuffer() {
    return this->W_par;
}

struct ts_t cSetup::getNextPacketTS(struct ts_t ts, struct ts_t sts, struct ts_t ets, u_int32_t srate, u_int32_t erate, u_int16_t len) {

    long delta_rate, usec_delta;
    u_int32_t sec, usec;
    usec_delta;
    u_int32_t delay;
    double interval;
    interval = (double) (ets.sec - sts.sec)+(double) (ets.usec - ets.usec) / 1000000.0;
    delta_rate = (int) (erate - srate);
    sec = ts.sec - sts.sec;
    usec = ts.usec - sts.usec;
    usec_delta = (sec * 1000000 + usec) / interval;
    delay = (u_int32_t) (1000000.0 / ((srate + (delta_rate * usec_delta) / 1000000.0) / (8.0 * len))); //interval [usec];
    //cout << "\t delay:" << delay << "\t srate:" << srate << "\t delta_rate:" << delta_rate << "\t interval:" << interval << "\t usec_delta:" << usec_delta << "\t sec:" << sec << "\t usec:" << usec << "\t len:" << len << endl;
    //cout << (delta_rate * usec_delta) / 1000000 << endl;
    //cout << (srate + (delta_rate * usec_delta) / 1000000.0) << endl;
    ts.sec = ts.sec + (ts.usec + delay) / 1000000;
    ts.usec = (ts.usec + delay) % 1000000;
    //usleep(500000); 
    return ts;
}

int cSetup::prepTimedBuffer() {
    u_int32_t s_tmp_rate, e_tmp_rate;
    u_int16_t tmp_len;
    ts_t tmp_ts, s_tmp_ts, e_tmp_ts;
    timed_packet_t tpacket;
    cerr << ".::. Packet generating started." << endl;
    if (!tpoints.empty()) {
        td_tmp = (tpoint_def_t) tpoints.front();
        s_tmp_rate = td_tmp.bitrate * 1000;
        s_tmp_ts.sec = (u_int64_t) floor(td_tmp.ts);
        s_tmp_ts.usec = (u_int64_t) (fmod(td_tmp.ts, 1)*1000000);
        tmp_len = td_tmp.len;
        tmp_ts = s_tmp_ts;
        cerr << "     ~ rate: " << td_tmp.bitrate << "\tts: " << td_tmp.ts << "\tsize: " << td_tmp.len << endl;
        tpoints.pop();
    } else {
        return 1;
    }

    while (!tpoints.empty()) {
        td_tmp = (tpoint_def_t) tpoints.front();
        cerr << "     ~ rate: " << td_tmp.bitrate << "\tts: " << td_tmp.ts << "\tsize: " << td_tmp.len << endl;

        e_tmp_ts.sec = (u_int64_t) floor(td_tmp.ts);
        e_tmp_ts.usec = (u_int64_t) (fmod(td_tmp.ts, 1)*1000000);
        e_tmp_rate = td_tmp.bitrate * 1000;
        while (longFromTS(tmp_ts) < longFromTS(e_tmp_ts)) {
            tmp_ts = this->getNextPacketTS(tmp_ts, s_tmp_ts, e_tmp_ts, s_tmp_rate, e_tmp_rate, tmp_len);
            tpacket.sec = tmp_ts.sec;
            tpacket.usec = tmp_ts.usec;
            tpacket.len = tmp_len;
            pbuffer.push(tpacket);
        }
        s_tmp_rate = e_tmp_rate;
        s_tmp_ts = e_tmp_ts;
        tmp_len = td_tmp.len;
        tpoints.pop();
    }
    while (longFromTS(tmp_ts) < (this->deadline * 1000000)) {
        tmp_ts = this->getNextPacketTS(tmp_ts, s_tmp_ts, e_tmp_ts, s_tmp_rate, e_tmp_rate, tmp_len);
        tpacket.sec = tmp_ts.sec;
        tpacket.usec = tmp_ts.usec;
        tpacket.len = tmp_len;
        pbuffer.push(tpacket);
    }
    cerr << "     ~ Total packets in send buffer: " << pbuffer.size() << endl;
    cerr << "     ~ Used memory by packets in send buffer: " << (pbuffer.size() * sizeof (struct timed_packet_t) / 1000000) << " MB" << endl;
    //cout << "     ~ Allocated memory for send buffer: " << (tBuffer->getAlocatedMemSize() / 1000000) << " MB" << endl;
    cerr << ".::. Packet generating finished" << endl << endl;
    sleep(2);
    return 0;
}

long cSetup::longFromTS(ts_t ts) {
    return ts.sec * 1000000 + ts.usec;
}

timed_packet_t cSetup::getNextPacket() {
    tmp_tpck = pbuffer.front();
    pbuffer.pop();
    return tmp_tpck;
}

bool cSetup::nextPacket() {
    return pbuffer.empty();
}

void cSetup::setExtFilename(string s) {
    this->extfilename = s;
}

string cSetup::getExtFilename() {
    return this->extfilename;
}

uint8_t cSetup::extFilenameLen() {
    return this->extfilename.length();
}

string cSetup::getInterface() {
    return this->interface;
}

bool cSetup::toCSV(void) {
    return C_par;
}

void cSetup::setCPAR(bool val) {
    C_par = val;
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

void cSetup::setAntiAsym(bool val){
    this->antiAsym=val;
}