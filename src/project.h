//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================

#ifndef	PROJECT_H
#define	PROJECT_H

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
#include <locale>
#include <vector>
#include <bitset>

// DEBUG PURPOSES
//#define NDEBUG
#include <cassert>

#define DEBUG true /*CHANGE TO FALSE for no debug printing*/
#define dprint(a,b) if(DEBUG) {cout << (a) << ": " << (b) << endl;}
#define dprintm(a,b) if(DEBUG) {cout << (a) << ": " << StatusMessage[(int)(b)] << endl;}

inline void printBinary(int a) {
	std::cout << std::bitset < 16 > (a) << " " << a << std::endl;
}

// END DEBUG

#define PORT_CLIENT 10062
#define PORT_SERVER 10063
#define PORT_EXTRA  10064

#define NOTHING 0
#define NO_CONTENT const_cast<char*>("NO_CNT")

#define PACKET_SIZE 256

enum class StatusResult {
	Success,
	Error,
	FatalError,
	NotInitialized,
	AlreadyInitialized,
	CouldNotOpen,
	ChecksumDoesNotMatch,
	NotExpectedType,
	Timeout
};

static std::string StatusMessage[] = { "Success", "Error", "FatalError", "NotInitialized", "AlreadyInitialized",
		"CouldNotOpen", "ChecksumDoesNotMatch", "NotExpectedType", "Timeout" };

#endif
