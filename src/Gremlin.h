//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================

#ifndef GREMLIN_H
#define GREMLIN_H

#include "project.h"

using namespace std;

class Gremlin {

private:
    static Gremlin *manager;
    double damage_prob, loss_prob;
    bool initialized;

public:
    Gremlin();
    StatusResult initialize(double damage_prob, double loss_prob);
    StatusResult tamper(char *buffer, size_t *bufflen);
    static Gremlin *instance() {
        if (manager == NULL) {
            manager = new Gremlin;
        }
        return manager;
    }
};

#endif //GREMLIN_H
