//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================

#include "packets.h"
#include "Sockets.h"

using namespace std;

Sockets *Sockets::manager = NULL;

Sockets::Sockets() {
    this->initialized = false;
    this->socket_id = -1;
    this->socket_ready = false;
    deft_timeout.tv_sec = 0;
    deft_timeout.tv_usec = 200000;
}

/**
 * Binds addresses and ports to sockaddr structures
 *
 * @param address_from IP Address of near client in dot format
 * @param address_to IP Address of far client in dot format
 * @param port_from Port integer of near client
 * @param port_to Port integer of far client
 * @return status of binding
 */
StatusResult Sockets::BindAddresses(string address_from, string address_to, uint16_t port_from, uint16_t port_to) {
    socket_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_id < 0) {
        perror("error on open socket");
        return StatusResult::CouldNotOpen;
    }

    struct in_addr adr;

    memset(&server_sock_addr, NOTHING, sizeof(server_sock_addr));
    server_sock_addr.sin_family = AF_INET;
    if (inet_aton(address_to.c_str(), &adr) == 0) {
        perror("Invalid to address");
        return StatusResult::CouldNotOpen;
    }
    server_sock_addr.sin_addr = adr;
    server_sock_addr.sin_port = htons(port_to);

    memset(&client_sock_addr, NOTHING, sizeof(client_sock_addr));
    client_sock_addr.sin_family = AF_INET;
    if (inet_aton(address_from.c_str(), &adr) == 0) {
        perror("Invalid from address");
        return StatusResult::CouldNotOpen;
    }
    client_sock_addr.sin_addr = adr;
    client_sock_addr.sin_port = htons(port_from);
    return StatusResult::Success;
}

/**
 * Binds addresses to socket. Far addresses is overwritten after receiving
 * from client
 *
 * @param address_from IP Address of near client in dot format
 * @param address_to IP Address of far client in dot format
 * @param port_from Port integer of near client
 * @param port_to Port integer of far client
 * @return status of binding to socket
 */
StatusResult Sockets::OpenServer(string address_from, string address_to, uint16_t port_from, uint16_t port_to) {
    if (initialized) {
        return StatusResult::AlreadyInitialized;
    }
    StatusResult bindr = BindAddresses(address_from, address_to, port_from, port_to);
    if (bindr != StatusResult::Success)
        return bindr;

    int res = bind(socket_id, (sockaddr *) &server_sock_addr, sizeof(server_sock_addr));
    if (res < 0) {
        perror("error binding server");
        return StatusResult::CouldNotOpen;
    }
    initialized = true;
    socket_ready = true;
    return StatusResult::Success;
}

/**
 * Connects addresses to socket.
 *
 * @param address_from IP Address of near client in dot format
 * @param address_to IP Address of far client in dot format
 * @param port_from Port integer of near client
 * @param port_to Port integer of far client
 * @return status of connecting to socket
 */
StatusResult Sockets::OpenClient(string address_from, string address_to, uint16_t port_from, uint16_t port_to) {
    if (initialized) {
        return StatusResult::AlreadyInitialized;
    }
    StatusResult bindr = BindAddresses(address_from, address_to, port_from, port_to);
    if (bindr != StatusResult::Success)
        return bindr;

    int res = connect(socket_id, (sockaddr *) &client_sock_addr, sizeof(client_sock_addr));
    if (res < 0) {
        perror("Error on connect");
        return StatusResult::CouldNotOpen;
    }
    initialized = true;
    socket_ready = true;
    return StatusResult::Success;
}

/**
 * Closes socket and invalidates object for reuse.
 */
void Sockets::Close() {
    if (initialized) {
        close(socket_id);
        initialized = false;
        socket_ready = false;
    }
}

/**
 * Blocks while waiting to receive a packet.
 *
 * @param buffer Packet buffer returned
 * @param bufflen Length of packet expected, actual length returned via
 * @return packet returned by buffer and its size in bufflen.
 * Status of receipt returned
 */
StatusResult Sockets::Receive(char *buffer, size_t *bufflen) {
    if (!initialized) {
        return StatusResult::NotInitialized;
    }
    memset(buffer, NOTHING, *bufflen); /*Clear buffer that packet will be put in*/
    socklen_t length = sizeof(client_sock_addr); /*length of struct*/
    size_t res = (size_t) recvfrom(socket_id, buffer, *bufflen, 0, (sockaddr *) &client_sock_addr, &length);
    if (res < 0) { /*If error occurred*/
        perror("Error on receive");
        return StatusResult::FatalError;
    }
    buffer[res] = 0; /*Set last byte to null*/
    *bufflen = res; /*Return length via pointer*/
    if (DEBUG) {
        cout << "recvd packet [begin]\n";
        fwrite(buffer, *bufflen, 1, stdout);
        cout << endl << "[end]" << endl;
    } /*DEBUG*/
    return StatusResult::Success;
}

/**
 * Blocks while waiting to receive a packet. Returns after default
 * Sockets timeout has elapses
 *
 * @param buffer Packet buffer returned
 * @param bufflen Length of packet expected, actual length returned via
 * @return packet returned by buffer and its size in bufflen.
 * Status of receipt returned
 */
StatusResult Sockets::ReceiveTimeout(char *buffer, size_t *bufflen) {
    ResetTimeout(rtt_determined.tv_sec,rtt_determined.tv_usec);
    return ReceiveTimeout(buffer, bufflen, deft_timeout);
}

/**
 * Blocks while waiting to receive a packet. Returns after timeout elapses
 *
 * @param buffer Packet buffer returned
 * @param bufflen Length of packet expected, actual length returned via
 * @param microsec Time to wait before returning
 * @return packet returned by buffer and its size in bufflen.
 * Status of receipt returned
 */
StatusResult Sockets::ReceiveTimeout(char *buffer, size_t *bufflen, timeval &timeout) {
    FD_ZERO (&socks);
    FD_SET(socket_id, &socks);
    int rval = select(socket_id + 1, &socks, NULL, NULL, &timeout);
    if (rval < 0) {
        perror("Receive select error");
        return StatusResult::FatalError;
    } else if (rval == 0) {
        return StatusResult::Timeout;
    } else {
        return Receive(buffer, bufflen);
    }
}

/**
 * Sends packet to socket
 *
 * @param buffer Packet buffer to send
 * @param bufflen Buffer length to send
 * @return Status of sending
 */
StatusResult Sockets::Send(char *buffer, size_t *bufflen) {
    if (!initialized) {
        return StatusResult::NotInitialized;
    }
    socklen_t length = sizeof(client_sock_addr);
    if (DEBUG) {
        cout << "sent packet [begin]\n";
        fwrite(buffer, *bufflen, 1, stdout);
        cout << endl << "[end]" << endl;
    }/*DEBUG*/
    ssize_t res = sendto(socket_id, buffer, *bufflen, 0, (sockaddr *) &client_sock_addr, length);
    if (res < 0) {
        perror("Error on send");
        return StatusResult::Error;
    }
    return StatusResult::Success;
}

Sockets::~Sockets() {
    Close();
}

int Sockets::TestRoundTrip(int side) {
    ResetTimeout(10, 0);
    int trips = 5;
    char dat[] = "start";
    long trip_times[5];
    StatusResult res;
    chrono::steady_clock::time_point start_time, end_time;
    if (side == CLIENT) {
        dprint("RTTT side", "client")
        RTTPacket client_send(ReqType::RTTClient, dat);
        RTTPacket server_resp(ReqType::RTTServer, dat);
        dprintm("[c] to server", client_send.Send())
        dprintm("[c] rcv server", res = server_resp.Receive())
        if (res == StatusResult::Timeout) {
            perror("Server did not respond to RTT start");
            return -1;
        }
    }

    if (side == SERVER) {
        ResetTimeout(10, 0);
        dprint("RTTT side", "server")
        RTTPacket from_client(ReqType::RTTClient);
        dprintm("[s] rcv client", res = from_client.Receive())
        if (res == StatusResult::Timeout) {
            perror("Client did not respond to RTT start");
            return -1;
        } else {
            RTTPacket my_resp(ReqType::RTTServer);
            dprintm("[s] to client", my_resp.Send())
        }
    }
    dprint("starting", "loop")
    while (trips-- > 0) {
        ResetTimeout(3, 0);
        char p_content[2];
        sprintf(p_content, "%d", trips);
        ReqType type_send = (side == CLIENT) ? ReqType::RTTServer : ReqType::RTTClient;
        RTTPacket init((side == SERVER) ? ReqType::RTTServer : ReqType::RTTClient, p_content);
        if (trips == 4) {
            dprintm("[li] send", init.Send())
            start_time = chrono::steady_clock::now();
        }
        RTTPacket in(type_send, p_content);
        dprintm("[l] rcv", res = in.Receive())
        if (res == StatusResult::Timeout) { /*Select timed out*/
            perror("SELECT timeout");
            trip_times[trips] = 0;
        } else { /*the socket is ready to be read from*/
            end_time = chrono::steady_clock::now();
            chrono::microseconds span = chrono::duration_cast<chrono::microseconds>(end_time - start_time);
            trip_times[trips] = span.count();
            type_send = (side == SERVER) ? ReqType::RTTServer : ReqType::RTTClient;
            RTTPacket out(type_send, p_content); /*Send opposite type of packet to other side*/
            dprintm("[l] send", out.Send())
            start_time = chrono::steady_clock::now();
        }
    }
    int average = 0, r_total = 0;
    for (int i = 0; i < 5; i++)
        r_total += trip_times[i];
    average = r_total / 5;
    rtt_determined.tv_usec = average;
    rtt_determined.tv_sec = 0;
    dprint("Average RTT", average)
    return average;
}

void Sockets::ResetTimeout(long int sec = TIMEOUT_SEC, long int micro_sec = TIMEOUT_MSEC) {
    deft_timeout.tv_sec = sec;
    deft_timeout.tv_usec = micro_sec;
}
