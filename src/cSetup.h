/* 
 * File:   cSetup.h
 * 
 * Author: Ondrej Vondrous
 * Email: vondrous@0xFF.cz
 *
 * Created on 26. červen 2012, 22:38
 */

#ifndef SETUP_H
#define	SETUP_H

#define MIN_INTERVAL 50        // 0.000001 s  //1us
#define MAX_INTERVAL 1000000    // 1s

#define SETUP_CHCK_ERR 0
#define SETUP_CHCK_VER 1
#define SETUP_CHCK_SHOW 2
#define SETUP_CHCK_OK 3

#include <sys/types.h>
#include <string>
#include <sstream>
#include <climits>
#include <stdlib.h>
#include <libio.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <vector>
#include <stdint.h>
#include "_types.h"

using namespace std;

class cSetup {
public:
    cSetup(int argc, char **argv, string version);
    void usage(void);
    void show_version(void);                             //get version;
    u_int16_t getMaxPacketSize();
    bool isServer(void);
    bool isClient(void);
    u_int8_t self_check(void);
    int getPort();
    bool debug(void);
    FILE * getFP(void);
    bool showTimeStamps(void);
    bool isAsym(void);
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
    string getFilename(void);
    string getSrcFilename();
    bool silent(void);
    bool showBitrate(void);
    bool showSendBitrate(void);
    u_int32_t getTime_t();
    u_int32_t getTime_T();
    double getTime_R();
    u_int16_t getPayloadSize();
    void setPayoadSize(u_int16_t);
    string getHostname();
    u_int32_t getDeadline();
    u_int64_t getCount();
    double getInterval();
    double getInterval_i();
    double getInterval_I();
    double getBchange();
    double getSchange();
    double getRTBitrate(u_int64_t ts);
    u_int64_t getPacketSize();
    u_int64_t getMinInterval(void);
    u_int64_t getMaxInterval(void);
    u_int64_t getBaseRate(void);
    int parseSrcFile();
    vector<time_def> r_tpoints;
    vector<time_def> tpoints;
    
    
    
    virtual ~cSetup();
private:
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
    bool D_par;
    bool w_par;
    bool n_par;
    bool s_par;
    bool S_par;
    bool X_par;
    bool r_par;
    bool R_par;
    bool P_par;
    bool e_par;
    bool E_par;
    bool Q_par;
    bool U_par;
    bool W_par;
    bool u_par;
    bool _par;
    int port;
    double interval_i;  // 1s
    double interval_I;  // 1s
    double time_t;           // 10s
    double time_T;           // 10s
    double time_R;           // 10s
    u_int16_t size;          // 64B Payload
    int rate_b;         // 1kbit/s
    int rate_B;         // 1kbit/s
    u_int32_t step;     // 1ms
    u_int32_t deadline;       // 0s
    u_int64_t count;   
    string filename,srcfile;
    string host;
    string version;
    FILE *fp;
    int32_t brate,erate;
    u_int64_t bts,ets;
    bool tp_exhausted;
    struct time_def td_tmp;
    double bchange;
    
};

#endif	/* SETUP_H */

