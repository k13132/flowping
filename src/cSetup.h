/* 
 * File:   cSetup.h
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
    bool shape(void);
    bool toCSV(void);
    bool toCSV(bool);
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
    int prepTimedBuffer();
    bool useTimedBuffer(void);
    bool useTimedBuffer(bool);
    timed_packet_t getNextPacket();
    bool nextPacket();
    virtual ~cSetup();


private:
    uint64_t debug_temp;
    bool v_par;
    bool a_par;
    bool A_par;
    bool p_par;
    bool d_par;
    bool I_par;
    bool i_par;
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
    bool fpsize_set;
    struct tpoint_def_t td_tmp;
    double bchange;
    //vector<tpoint_def_t> r_tpoints;
    queue<tpoint_def_t> tpoints;
    //vector<tpoint_def_t> tpoints_copy;
    //cTimedBuffer * tBuffer;
    queue<timed_packet_t> pbuffer;
    struct ts_t getNextPacketTS(struct ts_t ts, struct ts_t sts, struct ts_t ets, u_int32_t srate, u_int32_t erate, u_int16_t len); //return interval in usecs//
    uint64_t longFromTS(ts_t ts);
    double doubleFromTS(ts_t ts);
    timed_packet_t tmp_tpck;

    //refactor tpoint to match duration of test / defined scenario is repeated
    void refactorTPoints(void);
};

#endif	/* SETUP_H */

