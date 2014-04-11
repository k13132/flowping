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
pthread_t t_cSender, t_cReceiver, t_cReceiver_output, t_sServer;

void * t_helper_sServer(void * arg) {
    server = (cServer *) arg;
    server->run();
}

void * t_helper_cSender(void * arg) {
    //    cpu_set_t mask;
    //    pthread_t thread;
    //    thread = pthread_self();
    //    CPU_ZERO(&mask);
    //    CPU_SET(2, &mask); //To run on CPU 2
    //    pthread_setaffinity_np(thread, sizeof (cpu_set_t), &mask);
    client = (cClient *) arg;
    client->run_sender();
}

void * t_helper_cReceiver(void * arg) {
    //    cpu_set_t mask;
    //    pthread_t thread;
    //    thread = pthread_self();
    //    CPU_ZERO(&mask);
    //    CPU_SET(4, &mask); //To run on CPU 4
    //    pthread_setaffinity_np(thread, sizeof (cpu_set_t), &mask);
    client = (cClient *) arg;
    client->run_receiver();
}

void * t_helper_cReceiver_output(void * arg) {
    //    cpu_set_t mask;
    //    pthread_t thread;
    //    thread = pthread_self();
    //    CPU_ZERO(&mask);
    //    CPU_SET(6, &mask); //To run on CPU 6
    //    pthread_setaffinity_np(thread, sizeof (cpu_set_t), &mask);
    client = (cClient *) arg;
    client->run_receiver_output();
}

void signalHandler(int sig) {
    if ((sig == SIGQUIT) || (sig == SIGTERM) || (sig == SIGINT)) { //SIG 3 //15 //2   QUIT
        if (setup->isServer()) {
            server->terminate();
            cout << "Server shutdown initiated." << endl;
            sleep(2);
            pthread_cancel(t_sServer);
        } else {
            client->terminate();
            sleep(2);
            pthread_cancel(t_cSender);
            pthread_cancel(t_cReceiver);
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

    char str[80];
#ifdef xENV_32
    strcpy(str, "x86_32 1.2.2");
#endif    
#ifdef xENV_64
    strcpy(str, "x86_64 1.2.2");
#endif    

    strcat(str, " (");
    strcat(str, DD);
    strcat(str, " ");
    strcat(str, TT);
    strcat(str, ")");

    setup = new cSetup(argc, argv, str);

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
        setup->usage();
        cout << "SETUP_CHCK_CODE=" << setup->self_check() << endl;
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
        pthread_setconcurrency(3);

        if (setup->useTimedBuffer()) {
            if (setup->prepTimedBuffer() != 0) {
                exit(1);
            };
        }
        if (pthread_create(&t_cReceiver, NULL, t_helper_cReceiver, (void *) client) != 0) {
            perror("pthread_create");
            exit(1);
        }
        //pthread_setaffinity_np(t_cReceiver_output, sizeof (cpu_set_t), &mask);
        if (pthread_create(&t_cReceiver_output, NULL, t_helper_cReceiver_output, (void *) client) != 0) {
            perror("pthread_create");
            exit(1);
        }
        //pthread_setaffinity_np(t_cReceiver, sizeof (cpu_set_t), &mask);
        if (pthread_create(&t_cSender, NULL, t_helper_cSender, (void *) client) != 0) {
            perror("pthread_create");
            exit(1);
        }
        //pthread_setaffinity_np(t_cSender, sizeof (cpu_set_t), &mask);
        pthread_join(t_cSender, NULL);
        pthread_join(t_cReceiver, NULL);
        delete(client);
        if (setup->npipe()) {
            system("rm -f /tmp/flowping");
        }
    }
    delete(setup);
    cout << endl << ".::. Good bye!" << endl;
    return EXIT_SUCCESS;
}

