//============================================================================
// Project 2: Reliable FTP Using GBN over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// April 15, 2016
//============================================================================

#include "Gremlin.h"

using namespace std;

Gremlin *Gremlin::manager = NULL;

Gremlin::Gremlin() {
    this->initialized = false;
    this->damage_prob = 0;
    this->loss_prob = 0;
    this->delay_prob = 0;
    this->delay_time = 0;
}

SR Gremlin::initialize(double damage_prob, double loss_prob, double delay_prob, int delay_time) {
    if (initialized) {
        return SR::AlreadyInitialized;
    } else if (damage_prob < 0 || damage_prob > 1 || loss_prob < 0 || loss_prob > 1 ||
               delay_prob < 0 || delay_prob > 1 || delay_time < 0) {
        return SR::Error;
    }

    cout << "Gremlin: dam=" << damage_prob * 100 << "%  loss=" << loss_prob * 100 << "%  delay=" <<
    delay_prob * 100 << "% " << delay_time << "ms" << endl;

    this->damage_prob = damage_prob;
    this->loss_prob = loss_prob;
    this->delay_prob = delay_prob;
    this->delay_time = delay_time;
    this->initialized = true;
    return SR::Success;
}

SR Gremlin::tamper(char *buffer, size_t *buff_len) {
    if (!initialized) {
        return SR::NotInitialized;
    }

    // Deciding to delay the packet or not
    double delay_roll = (rand() % 100 + 1) / 100.0;
    if (delay_roll <= delay_prob) {
        cout << "Packet delayed" << endl;
        return SR::Delayed;
    }

    // Deciding to corrupt the packet or not.
    double tamper_roll = (rand() % 100 + 1) / 100.0;
    //dprint("Tamper Roll", tamper_roll)
    if (tamper_roll <= damage_prob) {
        cout << "Packet tampered with" << endl;
        double degree_roll = (rand() % 100 + 1) / 100.0;
        if (degree_roll <= 0.7) {  // 1 bit corrupted
            int byte_roll = rand() % static_cast<int>(*buff_len);
            //dprint("Bytes changed (1)", byte_roll)
            memset(buffer + byte_roll, '|', 1);
        } else if (degree_roll <= 0.9) { // 2 bits corrupted
            int byte_roll = rand() % (static_cast<int>(*buff_len) - 1);
            //dprint("Bytes changed (2)", byte_roll)
            memset(buffer + byte_roll, '|', 2);
        } else if (degree_roll <= 1) { // 3 bits corrupted
            int byte_roll = rand() % (static_cast<int>(*buff_len) - 2);
            //dprint("Bytes changed (3)", byte_roll)
            memset(buffer + byte_roll, '|', 3);
        }
    } //else dprint("Tamper Roll", "Failed")

    // Deciding to drop the packet or not.
    double drop_roll = (rand() % 100 + 1) / 100.0;
    //dprint("Drop Roll", drop_roll)
    if (drop_roll <= loss_prob) {
        cout << "Packet Dropped" << endl;
        return SR::Dropped;
    } //else dprint("Drop Roll", "Failed")

    return SR::Success;
}

chrono::milliseconds Gremlin::get_delay() {
    return chrono::milliseconds(delay_time);
}

void Gremlin::Close() {
    this->initialized = false;
    this->damage_prob = 0;
    this->loss_prob = 0;
    this->delay_prob = 0;
    this->delay_time = 0;
}



