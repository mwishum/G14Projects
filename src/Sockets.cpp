//============================================================================
// Project 2: Reliable FTP Using GBN over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// April 15, 2016
//============================================================================

#include "Sockets.h"

Sockets *Sockets::manager = NULL;
int Sockets::TOTAL_SENT = 0;

Sockets::Sockets() : initialized(false), socket_id(-1) {
    deft_timeout.tv_sec = TIMEOUT_SEC;
    deft_timeout.tv_usec = TIMEOUT_MICRO_SEC;
    memset(&client_sock_addr, 0, sizeof(client_sock_addr));
    TOTAL_SENT = 0;
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
        perror("Error on open socket");
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

    int res = ::bind(socket_id, (sockaddr *) &server_sock_addr, sizeof(server_sock_addr));
    if (res < 0) {
        perror("Error binding server");
        return SR::CouldNotOpen;
    }
    initialized = true;
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
        perror("Error on open socket");
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
    return SR::Success;
}

/**
 * Closes socket and invalidates object for reuse.
 */
void Sockets::Close() {
    if (initialized) {
        close(socket_id);
        initialized = false;
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
        cerr << "[FATAL] Packet too large (" << res << "), not processed." << endl;
        return SR::FatalError;
    }
    buf_length = (size_t) res; /*Return length via pointer*/
    dprint("received packet size", buf_length)
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
 * @param timeout Time to wait before returning (sec and micro sec)
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
        perror("[FATAL] Receive select error");
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
    TOTAL_SENT++;
    socklen_t length = sizeof(client_sock_addr);
    dprint("Sent packet with size", bufflen)
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
    UseTimeout(10, 0);
    int trips = 5;
    char dat[] = "start";
    long trip_times[5];
    SR res;
    TIME_METHOD::time_point start_time, end_time;
    if (side == CLIENT) {
        dprint("\nRTTT side", "client")
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
        UseTimeout(5, 0);
        dprint("\nRTTT side", "server")
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
        UseTimeout(3, 0);
        char p_content[2];
        sprintf(p_content, "%d", trips);
        ReqType type_send = (side == CLIENT) ? ReqType::RTTServer : ReqType::RTTClient;
        RTTPacket init((side == SERVER) ? ReqType::RTTServer : ReqType::RTTClient, p_content, strlen(p_content));
        if (trips == 4) {
            init.Send();
            start_time = TIME_METHOD::now();
        }
        RTTPacket in(type_send, p_content, strlen(p_content));
        res = in.Receive();

        if (res == SR::Timeout) { /* Select timed out */
            perror("RTT timeout");
            trip_times[trips] = 0;
        } else { /* successful receive */
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
        dprint("Getting the last one", "")
        res = last_one.Receive();
        dprintm("on server", res)
    }
    if (side == CLIENT) {
        RTTPacket last_one(ReqType::RTTClient, NO_CONTENT, 1);
        dprint("Getting the last one", "")
        res = last_one.Receive();
        dprintm("on client", res)
    }
    int average = 0, r_total = 0;
    for (int i = 0; i < 5; i++)
        r_total += trip_times[i];
    average = r_total / 5;

    rtt_determined.tv_usec = average * 3;
    rtt_determined.tv_sec = 0;
    cout << "============" << endl;
    cout << "Round trip time = " << rtt_determined.tv_usec << " microseconds ";
    chrono::milliseconds span = chrono::duration_cast<chrono::milliseconds>(chrono::microseconds(rtt_determined.tv_usec));
    cout << "(~" << span.count() << "ms)" << endl;
    cout << "============" << endl;
    UseTimeout(0, average * 3);
    return average;
}

/**
 * Sets the default timeout of this Sockets instance.
 *
 * @param sec Seconds of timeout
 * @param micro_sec Microseconds of timeout
 */
void Sockets::UseTimeout(long int sec, long int micro_sec) {
    deft_timeout.tv_sec = sec;
    deft_timeout.tv_usec = micro_sec;
}

/**
 * Blocks until a packet is received or Timeout reached.
 *
 *
 * @return result of awaiting.
 */
SR Sockets::AwaitPacket(char *packet_buf, size_t &buff_len, string &type) {
    if (!this->initialized) {
        return SR::NotInitialized;
    }

    SR rec = ReceiveTimeout(packet_buf, buff_len);

    if (rec != SR::Success) {
        return rec;
    }
    UnknownPacket packet(packet_buf, buff_len);

    type.clear();
    type = string(packet.type_string);
    //dprint("Await Packet Type:", type);
    return SR::Success;
}


/**
 * Blocks forever until a packet is received.
 *
 * @return result of awaiting.
 */
SR Sockets::AwaitPacketForever(char *packet_buf, size_t &buff_len, string &type) {
    if (!this->initialized) {
        return SR::NotInitialized;
    }

    SR rec = Receive(packet_buf, buff_len);

    if (rec != SR::Success) {
        return rec;
    }
    UnknownPacket packet(packet_buf, buff_len);

    type.clear();
    type = string(packet.type_string);
    return SR::Success;
}



