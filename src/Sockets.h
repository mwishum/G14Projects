//============================================================================
// Project 2: Reliable FTP Using GBN over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// April 15, 2016
//============================================================================

#ifndef SOCKETS_H_
#define SOCKETS_H_

#include "project.h"
#include "packets.h"
#include <chrono>
#include <memory>

#define CLIENT 10
#define SERVER 11

using namespace std;

class Sockets {

    static Sockets *manager;
    int socket_id;
    int side;
    struct sockaddr_in server_sock_addr;
    struct sockaddr_in client_sock_addr;
    socklen_t client_size;
    bool initialized;
    fd_set socks;
    struct timeval deft_timeout;

public:
    Sockets();
    virtual ~Sockets();
    SR OpenClient(string address_to, uint16_t port_to);
    SR OpenServer(string address_from, uint16_t port);
    void Close();
    SR Receive(char *buffer, size_t &buf_length);
    SR ReceiveTimeout(char *buffer, size_t &buff_len);
    SR ReceiveTimeout(char *buffer, size_t &buff_len, struct timeval timeout);
    SR AwaitPacket(char *packet_buf, size_t &buff_len, string &type);
    SR AwaitPacketForever(char *packet_buf, size_t &buff_len, string &type);
    SR Send(char *buffer, size_t &buff_len);
    int TestRoundTrip(int side);
    void UseTimeout(long int sec, long int micro_sec);
    int GetSide() { return side; }
    static Sockets *instance() {
        if (manager == NULL) {
            manager = new Sockets;
        }
        return manager;
    }
    struct timeval rtt_determined;
    static int TOTAL_SENT;
};

#endif /* SOCKETS_H_ */
