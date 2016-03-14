/* 
 * File:   flowping.cpp
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


#include <cstdlib>
#include <iostream>
#include "flowping.h"
#include <signal.h>
#include <sched.h>

using namespace std;

cSetup *setup = NULL;
cClient *client = NULL;
cServer *server = NULL;
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
            cout << "Server shutdown initiated." << endl;
            usleep(200000);
            pthread_cancel(t_sServer);
        } else {
            u_int16_t cnt;
            cnt=0;
            client->terminate();
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
        //zatim nedela nic
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
#ifdef xENV_32
    version << "x86_32 1.4.0e";
    version << " (" << DD << " "<< TT << ")";
#endif    
#ifdef xENV_64
    version << "x86_64 1.4.0e";
    version << " (" << DD << " "<< TT << ")";
#endif    

    //version << "ARM_32 1.4.0e" << " (" << DD << " "<< TT << ")";
    
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
        cout << "Invalid option!" << setup->self_check() << endl<< endl;
        setup->usage();
        return EXIT_FAILURE;
    }
    if (setup->raisePriority()) {
        struct sched_param param;
        param.sched_priority = sched_get_priority_max(SCHED_FIFO);
        sched_setscheduler(0, SCHED_FIFO, &param);
    }
    if (setup->isServer()) {
        server = new cServer(setup);
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
        client = new cClient(setup);
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

        timespec * tout;
        tout = new timespec;
        tout->tv_sec=2;
        tout->tv_nsec=0;
        pthread_timedjoin_np(t_cPacketFactory, NULL, tout);
        delete(client);
	delete(tout);
        if (setup->npipe()) {
            system("rm -f /tmp/flowping");
        }
    }
    delete(setup);
    cout << endl << ".::. Good bye!" << endl;
    return EXIT_SUCCESS;
}

