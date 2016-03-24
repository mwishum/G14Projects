//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================

#ifndef    PROJECT_H
#define    PROJECT_H

#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <string.h>
#include <sstream>
#include <locale>
#include <vector>
#include <bitset>

// DEBUG PURPOSES
//#define NDEBUG
#include <cassert>

#define DEBUG true /*CHANGE TO FALSE for no debug printing*/
#define dprint(a, b) if(DEBUG) {cout << (a) << ": " << (b) << endl;}
#define dprintm(a, b) if(DEBUG) {cout << (a) << ": " << StatusMessage[(int)(b)] << endl;}
#define dprintcmp(des, rec, act) if(DEBUG) {cout << (des) << " - Recd:" << (rec) << " Act:"<< (act) <<endl;}
#define dprintcmph(des, rec, act) if(DEBUG) {cout  << (des) << " - Recd:" << hex <<(rec) << " Act:"<< (act)  << dec <<endl;}

inline void printBinary(int a) {
    std::cout << std::bitset<16>(a) << " " << a << std::endl;
}

inline std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
        if (!item.empty())
            elems.push_back(item);
    }
    return elems;
}

// END DEBUG

#define PORT_CLIENT 10062
#define PORT_SERVER 10063
#define PORT_EXTRA  10064

#define NOTHING 0
#define NO_CONTENT const_cast<char*>("")

#define PACKET_SIZE 256

#define TIMEOUT_SEC 2
#define TIMEOUT_MSEC 0

#define MAX_LOOPS 150

enum class StatusResult {
    Success,
    Error,
    FatalError,
    NotInitialized,
    AlreadyInitialized,
    CouldNotOpen,
    ChecksumDoesNotMatch,
    NotExpectedType,
    OutOfSequence,
    Timeout,
    Dropped
};

static std::string StatusMessage[] = {
        "Success", "Error", "FatalError",
        "NotInitialized", "AlreadyInitialized", "CouldNotOpen",
        "ChecksumDoesNotMatch", "NotExpectedType", "OutOfSequence", "Timeout", "Dropped"};

#endif
