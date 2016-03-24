//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================

#ifndef SOCKETS_H_
#define SOCKETS_H_

#include "project.h"
#include "packets.h"
#include <chrono>

#define CLIENT 10
#define SERVER 11

using namespace std;

class Sockets {

    static Sockets *manager;
    int socket_id;
    int side;
    bool use_manual_timeout;
    struct sockaddr_in server_sock_addr;
    struct sockaddr_in client_sock_addr;
    bool socket_ready;
    bool initialized;
    fd_set socks;
    struct timeval deft_timeout, rtt_determined;

public:
    Sockets();
    StatusResult BindAddresses(string address_from, string address_to, uint16_t port_from, uint16_t port_to);
    virtual ~Sockets();
    StatusResult OpenServer(string address_from, string address_to, uint16_t port_from, uint16_t port_to);
    StatusResult OpenClient(string address_from, string address_to, uint16_t port_from, uint16_t port_to);
    void Close();
    StatusResult Receive(char *buffer, size_t *bufflen);
    StatusResult ReceiveTimeout(char *buffer, size_t *bufflen);
    StatusResult ReceiveTimeout(char *buffer, size_t *bufflen, timeval &timeout);
    StatusResult AwaitPacket(char *packet_buf, size_t *buff_len, string &type);
    StatusResult AwaitPacket(class Packet *packet, string &type);
    StatusResult Send(char *buffer, size_t *bufflen);
    int TestRoundTrip(int side);
    void ResetTimeout(long int sec, long int micro_sec);
    int GetSide() { return side; }
    static Sockets *instance() {
        if (manager == NULL) {
            manager = new Sockets;
        }
        return manager;
    }
};

#endif /* SOCKETS_H_ */
