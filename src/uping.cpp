/* 
 * File:   uping.cpp
 * 
 * Author: Ondrej Vondrous
 * Email: vondrous@0xFF.cz
 * 
 * Created on 26. červen 2012, 22:30
 */

#include <cstdlib>
#include <iostream>
#include "uping.h"
#include <signal.h>
#include <sched.h>

using namespace std;

cSetup *setup;
cClient *client = NULL;
cServer *server = NULL;
pthread_t t_cSender, t_cReceiver, t_sServer;

void * t_helper_sServer(void * arg) {
    server = (cServer *) arg;
    server->run();
}

void * t_helper_cSender(void * arg) {
    client = (cClient *) arg;
    client->run_sender();
}

void * t_helper_cReceiver(void * arg) {
    client = (cClient *) arg;
    client->run_receiver();
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



    cpu_set_t mask;
    pthread_t thread;
    thread = pthread_self();
    CPU_ZERO(&mask);
    CPU_SET(0, &mask); //To run on CPU 0

    pthread_setaffinity_np(thread, sizeof (cpu_set_t), &mask);
    //int result = sched_setaffinity(0, sizeof (mask), &mask);

#define DD __DATE__
#define TT __TIME__    

    char str[80];
#ifdef xENV_32
    strcpy(str, "x86_32 1.1.10");
#endif    
#ifdef xENV_64
    strcpy(str, "x86_64 1.1.10");
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
            if (access("/tmp/uping", F_OK) == -1) {
                int stat = mkfifo("/tmp/uping", 0700);
                if (stat != 0) {
                    fprintf(stdout, "Failed to create named pipe\n");
                    exit(-1);
                }

            }
        }
        client = new cClient(setup);
        pthread_setconcurrency(2);
        if (pthread_create(&t_cReceiver, NULL, t_helper_cReceiver, (void *) client) != 0) {
            perror("pthread_create");
            exit(1);
        }
        pthread_setaffinity_np(t_cReceiver, sizeof (cpu_set_t), &mask);
        if (pthread_create(&t_cSender, NULL, t_helper_cSender, (void *) client) != 0) {
            perror("pthread_create");
            exit(1);
        }
        pthread_setaffinity_np(t_cSender, sizeof (cpu_set_t), &mask);
        pthread_join(t_cSender, NULL);
        pthread_join(t_cReceiver, NULL);
        delete(client);
        if (setup->npipe()) {
            system("rm -f /tmp/uping");
        }
    }
    delete(setup);
    cout << "Good bye!" << endl;
    return EXIT_SUCCESS;
}

