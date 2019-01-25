/*
 * File:   flowping.cpp
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


#include "flowping.h"
#include <cstdlib>
#include <iostream>
#include <signal.h>
#include <sched.h>

using namespace std;

cSetup *setup = NULL;
cClient *client = NULL;
cServer *server = NULL;
cStats *stats = NULL;
pthread_t t_cSender, t_cReceiver, t_cReceiver_output, t_sServer, t_cPacketFactory;

void * t_helper_sServer(void * arg) {
    server = (cServer *) arg;
    server->run();
    return NULL;
}

//Send packets
void * t_helper_cSender(void * arg) {
    client = (cClient *) arg;
    client->run_sender();
    return NULL;
}

//Packet generator
void * t_helper_cPacketFactory(void * arg) {
    client = (cClient *) arg;
    client->run_packetFactory();
    return NULL;
}

//Receives packets
void * t_helper_cReceiver(void * arg) {
    client = (cClient *) arg;
    client->run_receiver();
    return NULL;
}

//Handle some basic signals
void signalHandler(int sig) {
    if ((sig == SIGQUIT) || (sig == SIGTERM) || (sig == SIGINT)) { //SIG 3 //15 //2   QUIT
        if (setup->isServer()) {
            server->terminate();
#ifdef DEBUG
            cerr << "Server shutdown initiated." << endl;
#endif
            usleep(200000);
            pthread_cancel(t_sServer);
        } else {
            u_int16_t cnt;
            cnt=0;
            client->terminate();
#ifdef DEBUG
            cerr << "Client shutdown initiated." << endl;
#endif            
            while ((client->status()&&(cnt<100))){
                usleep(50000);
                cnt++;
            }
            pthread_cancel(t_cSender);
            pthread_cancel(t_cReceiver);
            pthread_cancel(t_cPacketFactory);
        }
    }
    if (sig == SIGUSR1) { //SIG 10              
        if (stats){
            stats->printRealTime();
        }else{
            cerr << "Error: Stats module is not enabled. Recompile FlowPing with Stats module.\n";
        }
    }
    if (sig == SIGUSR2) { //SIG 12              
        //zatim nedela nic
    }
}

int main(int argc, char** argv) {
    // Osetreni reakci na signaly
    struct sigaction act;
    act.sa_handler = signalHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGQUIT, &act, 0);
    sigaction(SIGTERM, &act, 0);
    sigaction(SIGINT, &act, 0);
    sigaction(SIGUSR1, &act, 0);
    sigaction(SIGUSR2, &act, 0);

    //    cpu_set_t mask;
    //    pthread_t thread;
    //    thread = pthread_self();
    //    CPU_ZERO(&mask);
    //    CPU_SET(0, &mask); //To run on CPU 0
    //
    //    pthread_setaffinity_np(thread, sizeof (cpu_set_t), &mask);
    //int result = sched_setaffinity(0, sizeof (mask), &mask);

#define DD __DATE__
#define TT __TIME__    

    stringstream version;


    version.str("");
#ifdef __i386
    version << "x86_32 1.5.2a";
    version << " (" << DD << " "<< TT << ")";
#endif    
#ifdef __x86_64__
    version << "x86_64 1.5.2a";
    version << " (" << DD << " "<< TT << ")";
#endif    

#ifdef __ARM_ARCH_7A__
    version << "ARM_32 1.5.2a";
    version << " (" << DD << " "<< TT << ")";
#endif    
    
#ifdef __MIPS_ISA32__
    version << "MIPS_32 1.5.2a";
    version << " (" << DD << " "<< TT << ")";
#endif    

    
    setup = new cSetup(argc, argv, version.str());

    //Check cmd line parameters
    if (setup->self_check() == SETUP_CHCK_SHOW) {
        setup->usage();
        return EXIT_SUCCESS;
    }
    if (setup->self_check() == SETUP_CHCK_VER) {
        setup->show_version();
        if (setup->is_vonly()) return EXIT_SUCCESS;
    }
    if (setup->self_check() == SETUP_CHCK_ERR) {
        cerr << "Invalid option!" << setup->self_check() << endl<< endl;
        setup->usage();
        return EXIT_FAILURE;
    }
    if (setup->raisePriority()) {
        struct sched_param param;
        param.sched_priority = sched_get_priority_max(SCHED_FIFO);
        sched_setscheduler(0, SCHED_FIFO, &param);
    }
    if (setup->isServer()) {
#ifndef _NOSTATS        
        stats = new cServerStats(setup);
#endif
        server = new cServer(setup, stats);
        if (pthread_create(&t_sServer, NULL, t_helper_sServer, (void *) server) != 0) {
            perror("pthread_create");
            exit(1);
        }
        pthread_join(t_sServer, NULL);
        delete(server);
    } else {
        if (setup->npipe()) {
            if (access("/tmp/flowping", F_OK) == -1) {
                int stat = mkfifo("/tmp/flowping", 0700);
                if (stat != 0) {
                    fprintf(stdout, "Failed to create named pipe\n");
                    exit(-1);
                }

            }
        }
#ifndef _NOSTATS        
        stats = new cClientStats(setup);
#endif
        client = new cClient(setup, stats);
        pthread_setconcurrency(4);
        if (pthread_create(&t_cPacketFactory, NULL, t_helper_cPacketFactory, (void *) client) != 0) {
            perror("pthread_create");
            exit(1);
        }

        if (pthread_create(&t_cReceiver, NULL, t_helper_cReceiver, (void *) client) != 0) {
            perror("pthread_create");
            exit(1);
        }
        if (pthread_create(&t_cSender, NULL, t_helper_cSender, (void *) client) != 0) {
            perror("pthread_create");
            exit(1);
        }
        pthread_join(t_cSender, NULL);
        pthread_join(t_cReceiver, NULL);
        //pthread_join(t_cPacketFactory, NULL);

        //timespec * tout;
        //tout = new timespec;
        //tout->tv_sec=2;
        //tout->tv_nsec=0;
        //pthread_timedjoin_np(t_cPacketFactory, NULL, tout);
        delete(client);
	//delete(tout);
        if (setup->npipe()) {
            if (system("rm -f /tmp/flowping")){
            perror("npipe removal failed");
            exit(1);
            }
        }
    }
    delete(setup);
    delete(stats);
    //cerr << endl << ".::. Good bye!" << endl;
    return EXIT_SUCCESS;
}

