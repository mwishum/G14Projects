//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================

#include "Gremlin.h"

using namespace std;

Gremlin *Gremlin::manager = NULL;

Gremlin::Gremlin() {
    this->initialized = false;
    this->damage_prob = 0;
    this->loss_prob = 0;
}

StatusResult Gremlin::initialize(double damage_prob, double loss_prob) {
    if (initialized) {
        return StatusResult::AlreadyInitialized;
    } else if (damage_prob < 0 || damage_prob > 1 || loss_prob < 0 || loss_prob > 1) {
        return StatusResult::Error;
    }

    this->damage_prob = damage_prob;
    this->loss_prob = loss_prob;
    this->initialized = true;
    return StatusResult::Success;
}

StatusResult Gremlin::tamper(char *buffer, size_t *bufflen) {
    if (!initialized) {
        return StatusResult::NotInitialized;
    }

    // Deciding to corrupt the packet or not.
    double tamper_roll = (rand() % 100 + 1) / 100.0;
    //dprint("Tamper Roll", tamper_roll)
    if  (tamper_roll <= damage_prob) {
        dprint("====Tamper Roll====", "====Successful====")
        double degree_roll = (rand() % 100 + 1) / 100.0;
        if (degree_roll <= 0.7) {  // 1 bit corrupted
            int byte_roll = rand() % static_cast<int>(*bufflen);
            //dprint("Bytes changed (1)", byte_roll)
            memset(buffer + byte_roll, '|', 1);
        } else if (degree_roll <= 0.9) { // 2 bits corrupted
            int byte_roll = rand() % (static_cast<int>(*bufflen) - 1);
            //dprint("Bytes changed (2)", byte_roll)
            memset(buffer + byte_roll, '|', 2);
        } else if (degree_roll <= 1) { // 3 bits corrupted
            int byte_roll = rand() % (static_cast<int>(*bufflen) - 2);
            //dprint("Bytes changed (3)", byte_roll)
            memset(buffer + byte_roll, '|', 3);
        }
    } //else dprint("Tamper Roll", "Failed")

    // Deciding to drop the packet or not.
    double drop_roll = (rand() % 100 + 1) / 100.0;
    //dprint("Drop Roll", drop_roll)
    if (drop_roll <= loss_prob) {
        //dprint("====Drop Roll====", "====Successful====")
        return StatusResult::Dropped;
    } //else dprint("Drop Roll", "Failed")

    return StatusResult::Success;
}