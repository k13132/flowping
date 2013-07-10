/* 
 * File:   server.cpp
 * 
 * Author: Ondrej Vondrous
 * Email: vondrous@0xFF.cz
 * 
 * Created on 26. červen 2012, 22:37
 */

#include <stdlib.h>
#include "cServer.h"
#include "_types.h"

cServer::cServer(cSetup *setup) {
    this->setup = setup;
    this->stop = false;
}

cServer::~cServer() {
    close(this->sock);
}

int cServer::run() {
    struct sockaddr_in saServer;
    FILE * fp = setup->getFP();
    u_int64_t count = 0;
    bool show = not setup->silent();

    // Create a UDP socket
    if ((this->sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Can't open stream socket");
        exit(1);
    }
    // Fill in the address structure
    bzero((char *) & saServer, sizeof (saServer));
    saServer.sin_family = AF_INET;
    saServer.sin_addr.s_addr = htonl(INADDR_ANY); // Auto asssign address
    saServer.sin_port = htons(setup->getPort()); // Set listening port

    // bind to the socket
    int ret_size;
    int rec_size;
    rec_size = ret_size = bind(this->sock, (struct sockaddr *) & saServer, sizeof (saServer));

    if (ret_size < 0) {
        perror("Can't bind to local address");
        exit(1);
    }
    // Show the server name and port number
    //if (setup->debug()) {
    char hostname[200];
    ret_size = gethostname(hostname, sizeof (hostname));
    if (ret_size < 0) {
        perror("gethostname");
        exit(1);
    }
    printf("\nServer %s waiting on port %d\n", hostname, setup->getPort());

    //}

    // Wait for data
    int addr_len;
    addr_len = sizeof (sockaddr);
    struct Ping_Pkt *ping_pkt;
    struct Ping_Msg *ping_msg;
    unsigned char packet[MAX_PKT_SIZE + 60];
    struct sockaddr_in saClient;
    struct timeval curTv, refTv;
    refTv.tv_sec = 0;
    unsigned int cnt = 0;

    int ip;
    char client_ip[INET_ADDRSTRLEN];

    while (!stop) {
        rec_size = ret_size = recvfrom(this->sock, packet, MAX_PKT_SIZE, 0, (struct sockaddr *) &saClient, (socklen_t *) & addr_len);
#ifdef DEBUG        
        if (setup->debug()) cout << ret_size << " bytes received." << endl;
#endif        
        ping_pkt = (struct Ping_Pkt*) (packet);
        if (ping_pkt->type == PING) {
            if (setup->isAsym()) ret_size = MIN_PKT_SIZE;
            sendto(this->sock, packet, ret_size, 0, (struct sockaddr *) &saClient, addr_len);
        }
        if (ping_pkt->type == CONTROL) {
            ip = (saClient.sin_addr.s_addr);
            inet_ntop(AF_INET, &ip, client_ip, INET_ADDRSTRLEN);
            ping_msg = (struct Ping_Msg*) (packet);
#ifdef DEBUG
            if (setup->debug()) cout << "Control packet received! code:" << (int) ping_msg->code << endl;
#endif
            if (ping_msg->code == CNT_FNAME) {
                fp = fopen(ping_msg->msg, "w+"); //RW - overwrite file
                ping_msg->code = CNT_FNAME_OK;
                if (fp == NULL) {
                    perror("Unable to open file, redirecting to STDOUT");
                    fp = stdout;
                    ping_msg->code = CNT_OUTPUT_REDIR;
                } else {
                   if(setup->self_check() == SETUP_CHCK_VER) fprintf(fp, setup->get_version().c_str());
                }
            }
            if (ping_msg->code == CNT_NOFNAME) {
                if (setup->getFilename().length() && setup->outToFile()) {
                    fp = fopen(setup->getFilename().c_str(), "w+"); //RW - overwrite file
                    if (fp == NULL) {
                        perror("Unable to open file, redirecting to STDOUT");
                        fp = stdout;
                        ping_msg->code = CNT_OUTPUT_REDIR;
                    }
                } else {
                    fp = setup->getFP();
                }
                ping_msg->code = CNT_FNAME_OK;
            }
            if (ping_msg->code == CNT_DONE) {
                ping_msg->code = CNT_DONE_OK;
                ping_msg->count = count;
                if (fp != stdout) {
                    fclose(fp);
                    printf(".::. Test from %s finished.\n", client_ip);
                } else {
                    printf(".::. Test from %s finished.\n", client_ip);
                }
                fp = stdout;
                refTv.tv_sec = 0;
                count = 0;
            }
            ret_size = 64;
            sendto(this->sock, packet, ret_size, 0, (struct sockaddr *) &saClient, addr_len);
        }


        //stats
        if (ping_pkt->type == PING) {
            if (show) {
                if (refTv.tv_sec == 0) {
                    refTv.tv_sec = ping_pkt->sec;
                    refTv.tv_usec = ping_pkt->usec;
                } else {
                    refTv = curTv;
                }
                //curTv.tv_sec = ping_pkt->sec;
                //curTv.tv_usec = ping_pkt->usec;
                gettimeofday(&curTv, NULL);
                double delta = ((double) (curTv.tv_sec - refTv.tv_sec)*1000.0 + (double) (curTv.tv_usec - refTv.tv_usec) / 1000.0);
                if (setup->showTimeStamps()) {
                    fprintf(fp, "[%d.%06d] ", curTv.tv_sec, curTv.tv_usec);
                }

                fprintf(fp, "%d bytes from %s: req=%d ttl=xx delta=%.2f ms",
                        rec_size, client_ip, ping_pkt->seq, delta);
                if (setup->showBitrate()) {

                    fprintf(fp, " rx_rate=%.2f kbit/s ", (1000 / delta) * rec_size * 8 / 1000);
                    fprintf(fp, " tx_rate=%.2f kbit/s ", (1000 / delta) * ret_size * 8 / 1000);
                }
                fprintf(fp, "\n");

            }
            count++;
        }
    }
    return 1;
}

void cServer::terminate() {
    this->stop = true;
}

