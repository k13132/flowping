/*
 * File:   cSetup.h
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


#ifndef SETUP_H
#define	SETUP_H

#define MIN_INTERVAL 1000        // 0.000001 s  //1us
#define MAX_INTERVAL 1000000000    // 1s

#define SETUP_CHCK_ERR 0
#define SETUP_CHCK_VER 1
#define SETUP_CHCK_SHOW 2
#define SETUP_CHCK_OK 3

#include "_types.h"

using namespace std;

class cSetup {
public:
    cSetup(int argc, char **argv, string version);
    void usage(void);
    void show_version(void); //get version;
    u_int16_t getMaxPacketSize();
    u_int64_t getConnectionID(u_int32_t ip, uint16_t port);
    bool isServer(void);
    bool isClient(void);
    u_int8_t self_check(void);
    int getPort();
    FILE * getFP(void);
    bool showTimeStamps(void);
    bool showTimeStamps(bool);
    bool isAsym(void);
    bool isAsym(bool);
    bool isAntiAsym(void);
    bool isAntiAsym(bool);
    void setAntiAsym(bool val);
    bool sendFilename(void);
    bool outToFile(void);
    bool pkSizeChange(void);
    bool frameSize(void);
    bool compat(void);
    bool speedUP(void); //use fixed value for interval (do not compute for every packet)
    bool actWaiting(void); // use active waiting (do not use sleep(), usleep() functions) - more accurate timing
    bool raisePriority(void); // usech SHED FIFO and raise Priority.
    bool npipe(void);
    bool descFileInUse(void);
    bool toCSV(void);
    bool toCSV(bool);
    bool toJSON(void);
    bool toJSON(bool);
    void setCPAR(bool);
    void setXPAR(bool);
    void restoreXPAR(void);
    string getFilename(void);
    string getF_Filename(void);
    string getSrcFilename();
    bool silent(void);
    bool showBitrate(void);
    bool showBitrate(bool);
    bool showSendBitrate(void);
    bool showSendBitrate(bool);
    bool is_vonly(void);
    string get_version(void);
    u_int64_t getTime_t();
    u_int64_t getTime_T();
    double getTime_R();
    u_int16_t getPacketSize();
    u_int16_t getFirstPacketSize();
    void setPayoadSize(u_int16_t);
    string getHostname();
    u_int64_t getDeadline();
    u_int64_t getCount();
    double getInterval();
    double getInterval_i();
    double getInterval_I();
    double getBchange();
    double getSchange();
    bool wholeFrame(void);
    bool wholeFrame(bool);
    bool useInterface(void);
    string getInterface(void);
    void setHPAR(bool);
    void setWPAR(bool);
    double getRTBitrate(u_int64_t ts);
    u_int64_t getMinInterval(void);
    u_int64_t getMaxInterval(void);
    u_int64_t getBaseRate(void);
    void setExtFilename(string);
    string getExtFilename(void);
    uint8_t extFilenameLen(void);
    int parseSrcFile();
    int parseCmdLine();
    bool prepNextPacket();  //if false deadline reached
    bool useTimedBuffer(void);
    bool useTimedBuffer(bool);
    timed_packet_t getNextPacket();
    bool nextPacket();
    bool tpReady();
    u_int64_t getTimedBufferSize();
    u_int64_t getTimedBufferDelay();
    virtual ~cSetup();


private:
    pthread_mutex_t mutex;
    uint64_t debug_temp;
    bool v_par;
    bool a_par;
    bool A_par;
    bool p_par;
    bool d_par;
    bool I_par;
    bool i_par;
    bool J_par;
    bool t_par;
    bool T_par;
    bool b_par;
    bool B_par;
    bool q_par;
    bool f_par;
    bool h_par;
    bool H_par;
    bool F_par;
    bool c_par;
    bool C_par;
    bool D_par;
    bool w_par;
    bool n_par;
    bool s_par;
    bool S_par;
    bool X_par;
    bool XR_par;
    bool r_par;
    bool R_par;
    bool P_par;
    bool e_par;
    bool E_par;
    bool Q_par;
    bool U_par;
    bool W_par;
    bool u_par;
    bool vonly;
    bool _par;
    bool antiAsym;
    int port;
    double interval_i; // 1s
    double interval_I; // 1s
    double time_t; // 10s
    double time_T; // 10s
    double time_R; // 10s
    u_int16_t size, bsize, esize, fpsize; // 64B Payload
    int rate_b; // 1kbit/s
    int rate_B; // 1kbit/s
    u_int64_t step; // 
    u_int64_t deadline; // 0s
    u_int64_t count;
    string filename, F_filename, srcfile, extfilename;
    string host, interface;
    string version;
    FILE *fp;
    int64_t brate, erate;
    u_int64_t bts, ets;
    bool first_brate;
    bool tp_exhausted;
    bool tp_ready;
    bool fpsize_set;
    struct tpoint_def_t td_tmp;
    double bchange;
    double cumulative_delay;    //cumulative delay for zero bitrate definition
    queue<tpoint_def_t> tpoints;
    queue<timed_packet_t> pbuffer;
    struct ts_t getNextPacketTS(struct ts_t ts, struct ts_t sts, struct ts_t ets, u_int32_t srate, u_int32_t erate, u_int16_t len); //return interval in usecs//
    uint64_t longFromTS(ts_t ts);
    double doubleFromTS(ts_t ts);
    timed_packet_t tmp_tpck;

    //prepNextPacket
    u_int32_t s_tmp_rate, e_tmp_rate;
    u_int16_t tmp_len;
    ts_t tmp_ts, s_tmp_ts, e_tmp_ts;
    timed_packet_t tpacket;

    //getNextPacket
    long delta_rate, nsec_delta;
    u_int64_t delay;
    double interval, delta;

    //refactor tpoint to match duration of test / defined scenario is repeated
    void refactorTPoints(void);

    
};

#endif	/* SETUP_H */

