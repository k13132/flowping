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




#include "_types.h"
#include "cClient.h"
#include "cServer.h"
#include "cSetup.h"
#include "cStats.h"
#include "cMBroker.h"
#include "cSlotTimer.h"
#include <thread>

#include <cstdlib>
#include <iostream>
#include <csignal>
#include <sched.h>

cSetup *setup = nullptr;
cClient *client = nullptr;
cServer *server = nullptr;
cStats *stats = nullptr;
cMessageBroker * mbroker = nullptr;
cSlotTimer *stimer = nullptr;

void * t_helper_sServer(void * arg) {
    server = (cServer *) arg;
    server->run();
    return nullptr;
}

//Send packets
void * t_helper_cSender(void * arg) {
    client = (cClient *) arg;
    client->run_sender();
    return nullptr;
}

//Packet generator
void * t_helper_cPacketFactory(void * arg) {
    client = (cClient *) arg;
    client->run_packetFactory();
    return nullptr;
}

//Receives packets
void * t_helper_cReceiver(void * arg) {
    client = (cClient *) arg;
    client->run_receiver();
    return nullptr;
}

//Message Broker
void * t_helper_cMBroker(void * arg) {
    mbroker = (cMessageBroker *) arg;
    mbroker->run();
    return nullptr;
}

//Slot Timer
void * t_helper_cSlotTimer(void * arg) {
    stimer = (cSlotTimer *) arg;
    stimer->run();
    return nullptr;
}


//Handle some basic signals
void signalHandler(int sig) {
    if ((sig == SIGHUP) || (sig == SIGQUIT) || (sig == SIGTERM) || (sig == SIGINT)) { //SIG 3 //15 //2   QUIT
        if (setup->isServer()) {
            server->terminate();
        } else {
            u_int16_t cnt;
            cnt=0;
            client->terminate();
            while ((client->status()&&(cnt<100))){
                usleep(50000);
                cnt++;
            }
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
        if (stats){
            stats->printStatus();
        }else{
            cerr << "Error: Stats module is not enabled. Recompile FlowPing with Stats module.\n";
        }
    }
}

int main(int argc, char** argv) {
    // CPUs
    unsigned int cpus = std::thread::hardware_concurrency();

    // Osetreni reakci na signaly
    struct sigaction act;
    act.sa_handler = signalHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGHUP, &act, nullptr);
    sigaction(SIGQUIT, &act, nullptr);
    sigaction(SIGTERM, &act, nullptr);
    sigaction(SIGINT, &act, nullptr);
    sigaction(SIGUSR1, &act, nullptr);
    sigaction(SIGUSR2, &act, nullptr);

#define DD __DATE__
#define TT __TIME__    

    stringstream version;
    version.str("");

#ifdef __x86_64__
    version << "x86_64 2.9.0-dev .::. F-Tester edition .::.";
    version << " (" << DD << " "<< TT << ")";
#endif

#ifdef __arm__
    version << "arm 2.9.0-dev .::. F-Tester edition .::.";
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
        //if (setup->is_vonly()) return EXIT_SUCCESS;
        return EXIT_SUCCESS;
    }
    if (setup->self_check() == SETUP_CHCK_ERR) {
        cerr << "Invalid option!" << setup->self_check() << endl;
        //ToDo remove in other than F-Tester edition.
        if (setup->isClient()){
            std::cerr << "Missing [-J] parameter: Output to JSON is mandatory in FlowPing 2 F-Tester edition." << endl << endl;
        }
        setup->usage();
        return EXIT_FAILURE;
    }
    //Todo integrate SlotTimer in server code
    cpu_set_t cpuset;
    unsigned int cpu = 0;
    if (setup->isServer()) {
        stats = new cServerStats(setup);
        mbroker = new cMessageBroker(setup, stats);
        std::thread t_mBroker (t_helper_cMBroker, (void *) mbroker);
        //CPU_ZERO(&cpuset);
        //CPU_SET(cpu % cpus, &cpuset);
        //pthread_setaffinity_np(t_mBroker.native_handle(), sizeof(cpu_set_t), &cpuset);
        //cpu++;
        server = new cServer(setup, stats, mbroker);
        std::thread t_sServer (t_helper_sServer, (void *) server);
        //CPU_ZERO(&cpuset);
        //CPU_SET(cpu % cpus, &cpuset);
        //pthread_setaffinity_np(t_sServer.native_handle(), sizeof(cpu_set_t), &cpuset);
        //cpu++;
        t_sServer.join();
        delete(server);
        t_mBroker.join();
        delete(mbroker);
    } else {

        stats = new cClientStats(setup);
        mbroker = new cMessageBroker(setup, stats);
        stimer = new cSlotTimer(mbroker, setup);
        client = new cClient(setup, stats, mbroker, stimer);
        std::thread t_cPacketFactory (t_helper_cPacketFactory, (void *) client);
        //CPU_ZERO(&cpuset);
        //CPU_SET(cpu % cpus, &cpuset);
        //pthread_setaffinity_np(t_cPacketFactory.native_handle(), sizeof(cpu_set_t), &cpuset);
        //cpu++;

        std::thread t_mBroker (t_helper_cMBroker, (void *) mbroker);
        //CPU_ZERO(&cpuset);
        //CPU_SET(cpu % cpus, &cpuset);
        //pthread_setaffinity_np(t_mBroker.native_handle(), sizeof(cpu_set_t), &cpuset);
        ////cpu++;

        std::thread t_cSlotTimer (t_helper_cSlotTimer, (void *) stimer);
        //CPU_ZERO(&cpuset);
        //CPU_SET(cpu % cpus, &cpuset);
        //pthread_setaffinity_np(t_cSlotTimer.native_handle(), sizeof(cpu_set_t), &cpuset);
        //cpu++;
        std::thread t_cReceiver (t_helper_cReceiver, (void *) client);
        //CPU_ZERO(&cpuset);
        //CPU_SET(cpu % cpus, &cpuset);
        //pthread_setaffinity_np(t_cReceiver.native_handle(), sizeof(cpu_set_t), &cpuset);
        //cpu++;
        std::thread t_cSender (t_helper_cSender, (void *) client);
        //CPU_ZERO(&cpuset);
        //CPU_SET(cpu % cpus, &cpuset);
        //pthread_setaffinity_np(t_cSender.native_handle(), sizeof(cpu_set_t), &cpuset);
        //cpu++;
        t_cSender.join();
        t_cPacketFactory.join();
        t_cReceiver.join();
        delete(client);
        t_cSlotTimer.join();
        delete(stimer);
        t_mBroker.join();
        delete(mbroker);
    }
    delete(setup);
    delete(stats);
    return EXIT_SUCCESS;
}

