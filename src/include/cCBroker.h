//
// Created by Ondřej Vondrouš on 12.12.2022.
//

#ifndef FLOWPING_CCBROKER_H
#define FLOWPING_CCBROKER_H


#include "types.h"
#include <map>
#include <vector>
#include "cMBroker.h"

class cConnectionBroker {
public:
    cConnectionBroker(cMessageBroker *mbroker);
    virtual ~cConnectionBroker();
    conn_t *getConn(sockaddr_in6 saddr, ping_pkt_t *pkt, bool is_asym);
    conn_t *getConnByID(uint64_t conn_id) const;
    vector<uint64_t> getActiveConnIDs();
    uint16_t getConnCount() const;
    void gc(bool force);
    void run();
    void stop();
private:
    gen_msg_t * tmsg;
    cMessageBroker *mbroker;
    bool active;
    uint64_t gc_counter;
    uint16_t max_connections;
    map <uint64_t, conn_t *> connections;
    conn_t * conn;
    timespec tv;
};


#endif //FLOWPING_CCBROKER_H
