//============================================================================
// Project 2: Reliable FTP Using GBN over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// April 15, 2016
//============================================================================

#ifndef GREMLIN_H
#define GREMLIN_H

#include <chrono>
#include "project.h"

using namespace std;

class Gremlin {

private:
    static Gremlin *manager;
    double damage_prob, loss_prob, delay_prob;
    int delay_time;
    bool initialized;

public:
    Gremlin();
    SR initialize(double damage_prob, double loss_prob, double delay_prob, int delay_time);
    SR tamper(char *buffer, size_t *buff_len);
    static Gremlin *instance() {
        if (manager == NULL) {
            manager = new Gremlin;
        }
        return manager;
    }
    chrono::milliseconds get_delay();
    void Close();
};

#endif //GREMLIN_H
