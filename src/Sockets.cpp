//============================================================================
// Project 2: Reliable FTP Using GBN over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// April 15, 2016
//============================================================================

#include "Sockets.h"

using namespace std;

Sockets *Sockets::manager = NULL;

Sockets::Sockets() : initialized(false), socket_id(-1), socket_ready(false), use_manual_timeout(false) {
    deft_timeout.tv_sec = TIMEOUT_SEC;
    deft_timeout.tv_usec = TIMEOUT_MICRO_SEC;
}


/**
 * Binds addresses to socket. Far addresses is overwritten after receiving
 * from client
 *
 * @param address_from IP Address of near(THIS) client in dot format
 * @param port integer port number to connect with
 *
 * @return status of binding to socket
 */
SR Sockets::OpenServer(string address_from, uint16_t port) {
    if (initialized) {
        return SR::AlreadyInitialized;
    }
    side = SERVER;
    struct in_addr adr;

    socket_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_id < 0) {
        perror("error on open socket");
        return SR::CouldNotOpen;
    }

    memset(&server_sock_addr, NOTHING, sizeof(server_sock_addr));
    server_sock_addr.sin_family = AF_INET;
    if (inet_aton(address_from.c_str(), &adr) == 0) {
        perror("Invalid to address");
        return SR::CouldNotOpen;
    }
    server_sock_addr.sin_addr = adr;
    server_sock_addr.sin_port = htons(port);

    int res = bind(socket_id, (sockaddr *) &server_sock_addr, sizeof(server_sock_addr));
    if (res < 0) {
        perror("error binding server");
        return SR::CouldNotOpen;
    }
    initialized = true;
    socket_ready = true;
    return SR::Success;
}

/**
 * Connects addresses to socket.
 *
 * @param address_to IP Address of far client (Server) in dot format
 * @param port_to Port integer of far client (Server)
 *
 * @return status of connecting to socket
 */
SR Sockets::OpenClient(string address_to, uint16_t port_to) {
    if (initialized) {
        return SR::AlreadyInitialized;
    }
    side = CLIENT;
    struct in_addr adr;

    socket_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_id < 0) {
        perror("error on open socket");
        return SR::CouldNotOpen;
    }

    memset(&server_sock_addr, NOTHING, sizeof(server_sock_addr));
    server_sock_addr.sin_family = AF_INET;
    if (inet_aton(address_to.c_str(), &adr) == 0) {
        perror("Invalid to address");
        return SR::CouldNotOpen;
    }
    server_sock_addr.sin_addr = adr;
    server_sock_addr.sin_port = htons(port_to);

    int res = connect(socket_id, (sockaddr *) &server_sock_addr, sizeof(server_sock_addr));
    if (res < 0) {
        perror("Error on connect");
        return SR::CouldNotOpen;
    }
    initialized = true;
    socket_ready = true;
    return SR::Success;
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
 * @param buf_length Length of packet expected, actual length returned via.
 *
 * @return packet returned by buffer and its size in buf_length. Status of receipt returned.
 */
SR Sockets::Receive(char *buffer, size_t &buf_length) {
    if (!initialized) {
        return SR::NotInitialized;
    }
    assert(buffer != NULL);
    assert(buf_length > 0 && buf_length <= PACKET_SIZE);

    /*Clear buffer that packet will be put in*/
    memset(buffer, NOTHING, buf_length);
    ssize_t res;

    if (side == CLIENT) {
        res = recv(socket_id, buffer, buf_length, 0);
    } else {//SERVER
        res = recvfrom(socket_id, buffer, buf_length, 0, (sockaddr *) &client_sock_addr, &client_size);
    }

    if (res < 0) { /*If error occurred*/
        perror("Error on receive");
        return SR::FatalError;
    }
    buffer[res] = 0; /*Set last byte to null*/
    if (res > PACKET_SIZE) {
        cerr << "PACKET TOO LARGE (" << res << "), DROPPED" << endl;
        return SR::FatalError;
    }
    buf_length = (size_t) res; /*Return length via pointer*/
    dprintm("received packet size", buf_length)
    return SR::Success;
}

/**
 * Blocks execution while waiting to receive a packet. Returns after the default
 * Sockets.cpp timeout has elapsed.
 *
 * @param buffer Packet buffer returned
 * @param bufflen Length of packet expected, actual length returned via
 *
 * @return packet returned by buffer and its size in bufflen. Status of receipt returned
 */
SR Sockets::ReceiveTimeout(char *buffer, size_t &bufflen) {
    return ReceiveTimeout(buffer, bufflen, deft_timeout);
}

/**
 * Blocks while waiting to receive a packet. Returns after timeout elapses
 *
 * @param buffer Packet buffer returned
 * @param bufflen Length of packet expected, actual length returned via
 * @param timeout Time to wait before returning (sec and mirco sec)
 *
 * @return packet returned by buffer and its size in bufflen. Status of receipt returned
 */
SR Sockets::ReceiveTimeout(char *buffer, size_t &bufflen, struct timeval timeout) {
    FD_ZERO (&socks);
    FD_SET(socket_id, &socks);
    struct timeval temp;
    temp.tv_usec = timeout.tv_usec;
    temp.tv_sec = timeout.tv_sec;
    int rval = select(socket_id + 1, &socks, NULL, NULL, &temp);
    if (rval < 0) {
        perror("Receive select error");
        return SR::FatalError;
    } else if (rval == 0) {
        return SR::Timeout;
    } else {
        return Receive(buffer, bufflen);
    }
}

/**
 * Sends packet to socket
 *
 * @param buffer Packet buffer to send
 * @param bufflen Buffer length to send
 *
 * @return Status of sending
 */
SR Sockets::Send(char *buffer, size_t &bufflen) {
    if (!initialized) {
        return SR::NotInitialized;
    }
    socklen_t length = sizeof(client_sock_addr);
    if (DEBUG) {
        cout << "sent packet with size " << bufflen << endl;
    }/*DEBUG*/
    ssize_t res = 0;
    if (side == CLIENT)
        res = send(socket_id, buffer, bufflen, 0);
    else //SERVER
        res = sendto(socket_id, buffer, bufflen, 0, (sockaddr *) &client_sock_addr, length);

    if (res < 0) {
        perror("Error on send");
        return SR::Error;
    }
    return SR::Success;
}

Sockets::~Sockets() {
    Close();
}

/**
 * Tests the mean round trip time between client and server
 *
 * @param side Integer representing client or server
 *
 * @return int Average RTT
 */
int Sockets::TestRoundTrip(int side) {
    use_manual_timeout = true;
    ResetTimeout(10, 0);
    int trips = 5;
    char dat[] = "start";
    long trip_times[5];
    SR res;
    TIME_METHOD::time_point start_time, end_time;
    if (side == CLIENT) {
        dprint("RTTT side", "client")
        RTTPacket client_send(ReqType::RTTClient, dat, strlen(dat));
        RTTPacket server_resp(ReqType::RTTServer, dat, strlen(dat));
        client_send.Send();

        res = server_resp.Receive();
        if (res == SR::Timeout) {
            perror("Server did not respond to RTT start");
            return -1;
        } else if (res != SR::Success) {
            dprintm("Failed communicating with server", res)
            return -1;
        }
    }

    if (side == SERVER) {
        ResetTimeout(10, 0);
        dprint("RTTT side", "server")
        RTTPacket from_client(ReqType::RTTClient);
        res = from_client.Receive();

        if (res == SR::Timeout) {
            perror("Client did not respond to RTT start");
            return -1;
        } else if (res == SR::Success) {
            RTTPacket my_resp(ReqType::RTTServer);
            my_resp.Send();

        } else {
            dprintm("Failed communicating with client", res)
            return -1;
        }
    }
    dprint("starting", "RTT loop")
    while (trips-- > 0) {
        ResetTimeout(3, 0);
        char p_content[2];
        sprintf(p_content, "%d", trips);
        ReqType type_send = (side == CLIENT) ? ReqType::RTTServer : ReqType::RTTClient;
        RTTPacket init((side == SERVER) ? ReqType::RTTServer : ReqType::RTTClient, p_content, strlen(p_content));
        if (trips == trips - 1) {
            init.Send();
            start_time = TIME_METHOD::now();
        }
        RTTPacket in(type_send, p_content, strlen(p_content));
        res = in.Receive();

        if (res == SR::Timeout) { /* Select timed out */
            perror("RTT timeout");
            trip_times[trips] = 0;
        } else { /* the socket is ready to be read from */
            end_time = TIME_METHOD::now();
            chrono::microseconds span = chrono::duration_cast<chrono::microseconds>(end_time - start_time);
            trip_times[trips] = span.count();
            type_send = (side == SERVER) ? ReqType::RTTServer : ReqType::RTTClient;
            RTTPacket out(type_send, p_content, strlen(p_content)); /*Send opposite type of packet to other side*/
            out.Send();

            start_time = TIME_METHOD::now();
        }
    }
    if (side == SERVER) {
        RTTPacket last_one(ReqType::RTTServer, NO_CONTENT, 1);
        res = last_one.Receive();
        dprintm("Getting the last one", res)
    }
    if (side == CLIENT) {
        RTTPacket last_one(ReqType::RTTClient, NO_CONTENT, 1);
        res = last_one.Receive();
        dprintm("Getting the last one", res)
    }
    int average = 0, r_total = 0;
    for (int i = 0; i < 5; i++)
        r_total += trip_times[i];
    average = r_total / 5;

    rtt_determined.tv_usec = average * 2;
    rtt_determined.tv_sec = 0;
    cout << "Round trip time = " << average << " microseconds" << endl;

    use_manual_timeout = false;
    ResetTimeout(0, 0);
    return average;
}

/**
 * Resets the timeout of recv/send calls
 * Calling this resets the default timeout to the rtt determined one
 * UNLESS use_manual_timeout is called
 *
 * @param sec Seconds of timeout
 * @param micro_se Microseconds of timeout
 */
void Sockets::ResetTimeout(long int sec, long int micro_sec) {
    if (use_manual_timeout) {
        deft_timeout.tv_sec = sec;
        deft_timeout.tv_usec = micro_sec;
    }
    else {
        deft_timeout.tv_sec = rtt_determined.tv_sec;
        deft_timeout.tv_usec = rtt_determined.tv_usec;
    }
}

/**
 * Blocks until a packet is received or Timeout reached.
 *
 * @param packet Pointer to a pointer of the packet received
 * @param type Single letter string representing type of packet
 *
 * @return result of awaiting.
 */
SR Sockets::AwaitPacket(UnknownPacket **packet, string &type) {
    if (!this->initialized) return SR::NotInitialized;
    char buffer[PACKET_SIZE];
    size_t buffer_len = PACKET_SIZE;
    SR rec = ReceiveTimeout(buffer, buffer_len);

    if (rec != SR::Success) return rec;
    *packet = new UnknownPacket(buffer, buffer_len);
    assert(packet != nullptr);
    type.clear();
    type.insert(0, (*packet)->type_string, 1);
    dprint("await_packet_type", (*packet)->type_string)
    return SR::Success;
}


/**
 * Blocks forever until a packet is received.
 *
 * @param **packet Pointer to a pointer of the packet received
 * @param &type Single letter string representing type of packet
 *
 * @return result of awaiting.
 */
SR Sockets::AwaitPacketForever(UnknownPacket **packet, string &type) {
    if (!this->initialized) return SR::NotInitialized;
    char buffer[PACKET_SIZE];
    size_t buffer_len = PACKET_SIZE;
    SR rec = Receive(buffer, buffer_len);

    if (rec != SR::Success) return rec;
    *packet = new UnknownPacket(buffer, buffer_len);
    assert(packet != nullptr);
    type.clear();
    type.insert(0, (*packet)->type_string, 1);
    dprint("await_packet_type", (*packet)->type_string)
    return SR::Success;
}



