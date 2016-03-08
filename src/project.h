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
#include <string.h>
#include <locale>
#include <vector>

#define PORT_CLIENT 10062
#define PORT_SERVER_LINK 10063
#define PORT_SERVER_DATA 10064

#define NOTHING 0

#define PACKET_SIZE 256

#endif
