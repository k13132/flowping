/*
 * File:   cSetup.h
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


#ifndef SETUP_H
#define	SETUP_H

#define MIN_INTERVAL 1000        // 0.000001 s  //1us
#define MAX_INTERVAL 1000000000    // 1s

#define SETUP_CHCK_ERR 0
#define SETUP_CHCK_VER 1
#define SETUP_CHCK_SHOW 2
#define SETUP_CHCK_OK 3



#include "types.h"
#include "SPSCQueue.h"

using namespace std;

class cSetup {
public:
    cSetup(int argc, char **argv, string version);
    void usage(void);
    void show_version(void); //get version;
    uint16_t getMaxPacketSize();

    bool isServer(void);
    bool isClient(void);
    uint8_t self_check(void);
    int getPort();
    FILE * getFP(void);
    std::ostream* getOutput(void);
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
    bool actWaiting(void); // use active waiting (do not use sleep(), usleep() functions) - more accurate timing
    bool descFileInUse(void);
    bool toCSV(void);
    bool toJSON(void);
    void setCPAR(bool);
    void setJPAR(bool);
    void setXPAR(bool);
    void restoreXPAR(void);
    string getFilename(void);
    string getF_Filename(void);
    string getSrcFilename();
    bool silent(void);
    bool showBitrate(void);
    bool showSendBitrate(void);
    bool is_vonly(void);
    string get_version(void);
    string getVersion(void);
    void setAddrFamily(sa_family_t family);
    sa_family_t getAddrFamily(void);
    uint64_t getTime_t();
    uint64_t getTime_T();
    double getTime_R();
    uint16_t getPacketSize();
    uint16_t getFirstPacketSize();
    void setPayoadSize(uint16_t);
    string getHostname();
    string getWorkingDirectory();
    uint64_t getDeadline();
    uint64_t getCount();
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
    void setLPAR(bool);
    double getRTBitrate(uint64_t ts);
    uint64_t getMinInterval(void);
    uint64_t getMaxInterval(void);
    uint64_t getBaseRate(void);
    void setExtFilename(string);
    string getExtFilename(void);
    uint16_t extFilenameLen(void);
    int parseSrcFile();
    int parseCmdLine();
    bool prepNextPacket();  //if false deadline reached
    timed_packet_t getNextPacket();
    bool nextPacket();
    bool tpReady();
    uint64_t getTimedBufferSize();
    virtual ~cSetup();
    timed_packet_t get_tmp_tpck();
    void recordLastDelay(timespec last_delay);
    timespec getLastDelay();

    bool isStarted() const;
    bool isStop() const;
    bool isDone() const;
    bool isIPv6Prefered() const;
    void setStarted(bool started);
    void setStop(bool stop);
    void setDone(bool done);
    uint64_t getSampleLen(void) const;
    void setFlowID(uint16_t flow_id);
    uint16_t getFlowID();
    int getNetType();
    uint64_t getMaxTimedBufferSize();
    int getSocketRcvBufferSize();
    int getSocketSndBufferSize();
    uint16_t getMaxInvitePackets();
    uint32_t getInvitePacketRepeatDelay();

private:
    net_constants_t netConstants;
    uint64_t sample_len; //in ns ... 0 means no sampling
    sa_family_t addr_family = AF_INET;
    uint16_t flow_id;
    bool started, stop, done;
    bool v_par;
    bool a_par;
    bool p_par;
    bool d_par;
    bool I_par;
    bool i_par;
    bool J_par;
    bool L_par;
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
    bool W_par;
    bool s_par;
    bool S_par;
    bool X_par;
    bool XR_par;
    bool r_par;
    bool R_par;
    bool e_par;
    bool E_par;
    bool u_par;
    bool nettype_par;
    bool so_rcvbuf_par;
    bool so_sndbuf_par;
    bool net_invite_max_par;
    bool net_invite_repeat_delay_par;

    bool vonly;
    bool _par;
    bool antiAsym;
    int port;
    int net_type;
    int so_rcvbuf;
    int so_sndbuf;
    uint16_t net_invite_max;
    uint32_t net_invite_repeat_delay;
    double interval_i; // 1s
    double interval_I; // 1s
    double time_t; // 10s
    double time_T; // 10s
    double time_R; // 10s
    uint16_t size, bsize, esize, fpsize; // 64B Payload
    int rate_b; // 1kbit/s
    int rate_B; // 1kbit/s
    uint64_t step; // 
    uint64_t deadline; // 0s
    uint64_t count;
    string filename, F_filename, srcfile, extfilename, wdir;
    string host, interface;
    string version;
    FILE *fp;
    std::ofstream fout;
    std::ostream* output;
    int64_t brate, erate;
    uint64_t bts, ets;
    bool tp_ready;
    bool fpsize_set;
    struct tpoint_def_t td_tmp;
    queue<tpoint_def_t> tpoints;
    rigtorp::SPSCQueue<timed_packet_t> pbuffer {128000};
    uint64_t getNextPacketTS(uint64_t ts, uint64_t sts, uint64_t ets, uint64_t srate, uint64_t erate, uint16_t len);
    timed_packet_t tmp_tpck;
    timespec last_delay;
    
    //prepNextPacket
    uint64_t s_tmp_rate, e_tmp_rate;
    uint16_t tmp_len;
    uint64_t tmp_ts, s_tmp_ts, e_tmp_ts;
    timed_packet_t tpacket;

    //getNextPacket
    long delta_rate;
    uint64_t delay, nsec_delta;
    double interval;
    uint64_t tp_diff;
    bool ipv6;
    //refactor tpoint to match duration of test / defined scenario is repeated
    void refactorTPoints(void);
    static int parseNetType(const string &);
    void optimizeNetSettings(void);
};

#endif	/* SETUP_H */

