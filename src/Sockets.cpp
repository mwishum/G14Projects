//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================

#include "packets.h"

using namespace std;

Sockets *Sockets::manager = NULL;

Sockets::Sockets() : initialized(false), socket_id(-1), socket_ready(false), use_manual_timeout(false) {
    deft_timeout.tv_sec = TIMEOUT_SEC;
    deft_timeout.tv_usec = TIMEOUT_MSEC;
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
    side = SERVER;
    struct in_addr adr;

    socket_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_id < 0) {
        perror("error on open socket");
        return StatusResult::CouldNotOpen;
    }

    memset(&server_sock_addr, NOTHING, sizeof(server_sock_addr));
    server_sock_addr.sin_family = AF_INET;
    if (inet_aton(address_from.c_str(), &adr) == 0) {
        perror("Invalid to address");
        return StatusResult::CouldNotOpen;
    }
    server_sock_addr.sin_addr = adr;
    server_sock_addr.sin_port = htons(port_to);

//    memset(&client_sock_addr, NOTHING, sizeof(client_sock_addr));
//    client_sock_addr.sin_family = AF_INET;
//    if (inet_aton(address_to.c_str(), &adr) == 0) {
//       perror("Invalid from address");
//      return StatusResult::CouldNotOpen;
//    }
//    client_sock_addr.sin_addr = adr;
//    client_sock_addr.sin_port = htons(port_to);
//    client_size = sizeof(client_sock_addr);

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
    side = CLIENT;
    struct in_addr adr;

    socket_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_id < 0) {
        perror("error on open socket");
        return StatusResult::CouldNotOpen;
    }

    memset(&server_sock_addr, NOTHING, sizeof(server_sock_addr));
    server_sock_addr.sin_family = AF_INET;
    if (inet_aton(address_to.c_str(), &adr) == 0) {
        perror("Invalid to address");
        return StatusResult::CouldNotOpen;
    }
    server_sock_addr.sin_addr = adr;
    server_sock_addr.sin_port = htons(port_to);

    int res = connect(socket_id, (sockaddr *) &server_sock_addr, sizeof(server_sock_addr));
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
 * @param bufflen Length of packet expected, actual length returned via.
 *
 * @return packet returned by buffer and its size in bufflen. Status of receipt returned.
 */
StatusResult Sockets::Receive(char *buffer, size_t *bufflen) {
    if (!initialized) {
        return StatusResult::NotInitialized;
    }
    assert(buffer != NULL);
    assert(*bufflen > 0 && *bufflen <= PACKET_SIZE);
    memset(buffer, NOTHING, *bufflen); /*Clear buffer that packet will be put in*/
    ssize_t res;

    if (side == CLIENT) {
        res = recv(socket_id, buffer, *bufflen, 0);
    } else {//SERVER
        res = recvfrom(socket_id, buffer, *bufflen, 0, (sockaddr *) &client_sock_addr, &client_size);
        if (&client_sock_addr != NULL) dprint("client sock set", true)
    }

    if (res < 0) { /*If error occurred*/
        perror("Error on receive");
        return StatusResult::FatalError;
    }
    buffer[res] = 0; /*Set last byte to null*/
    if (res > PACKET_SIZE) {
        cerr << "PACKET TOO LARGE (" << res << "), DROPPED" << endl;
        return StatusResult::FatalError;
    }
    *bufflen = (size_t) res; /*Return length via pointer*/
    if (DEBUG) {
        cout << "recvd packet [begin]";
        //fwrite(buffer, *bufflen, 1, stdout);
        dprint("buff len", *bufflen)
        cout << " " << "[end]" << endl;
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
StatusResult Sockets::ReceiveTimeout(char *buffer, size_t *bufflen, struct timeval timeout) {
    FD_ZERO (&socks);
    FD_SET(socket_id, &socks);
    struct timeval temp;
    temp.tv_usec = timeout.tv_usec;
    temp.tv_sec = timeout.tv_sec;
    int rval = select(socket_id + 1, &socks, NULL, NULL, &temp);
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
        cout << "sent packet [begin]";
        //fwrite(buffer, *bufflen, 1, stdout);
        dprint("buff len", *bufflen)
        cout << " " << "[end]" << endl;
    }/*DEBUG*/
    ssize_t res = 0;
    if (side == CLIENT)
        res = send(socket_id, buffer, *bufflen, 0);
    else //SERVER
        res = sendto(socket_id, buffer, *bufflen, 0, (sockaddr *) &client_sock_addr, length);

    if (res < 0) {
        perror("Error on send");
        return StatusResult::Error;
    }
    return StatusResult::Success;
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
    StatusResult res;
    TIME_METHOD::time_point start_time, end_time;
    if (side == CLIENT) {
        dprint("RTTT side", "client")
        RTTPacket client_send(ReqType::RTTClient, dat, strlen(dat));
        RTTPacket server_resp(ReqType::RTTServer, dat, strlen(dat));
        res = client_send.Send();
        dprintm("[c] to server", res)
        res = res = server_resp.Receive();
        dprintm("[c] rcv server", res)
        if (res == StatusResult::Timeout) {
            perror("Server did not respond to RTT start");
            return -1;
        } else if (res != StatusResult::Success) {
            dprintm("Failed communicating with server", res)
            return -1;
        }
    }

    if (side == SERVER) {
        ResetTimeout(10, 0);
        dprint("RTTT side", "server")
        RTTPacket from_client(ReqType::RTTClient);
        res = from_client.Receive();
        dprintm("[s] rcv client", res)
        if (res == StatusResult::Timeout) {
            perror("Client did not respond to RTT start");
            return -1;
        } else if (res == StatusResult::Success) {
            RTTPacket my_resp(ReqType::RTTServer);
            res = my_resp.Send();
            dprintm("[s] to client", res)
        } else {
            dprintm("Failed communicating with client", res)
            return -1;
        }
    }
    dprint("starting", "loop")
    while (trips-- > 0) {
        ResetTimeout(3, 0);
        char p_content[2];
        sprintf(p_content, "%d", trips);
        ReqType type_send = (side == CLIENT) ? ReqType::RTTServer : ReqType::RTTClient;
        RTTPacket init((side == SERVER) ? ReqType::RTTServer : ReqType::RTTClient, p_content, strlen(p_content));
        if (trips == 4) {
            res = init.Send();
            dprintm("[li] send", res)
            start_time = TIME_METHOD::now();
        }
        RTTPacket in(type_send, p_content, strlen(p_content));
        res = in.Receive();
        dprintm("[l] rcv", res)
        if (res == StatusResult::Timeout) { /*Select timed out*/
            perror("SELECT timeout");
            trip_times[trips] = 0;
        } else { /*the socket is ready to be read from*/
            end_time = TIME_METHOD::now();
            chrono::microseconds span = chrono::duration_cast<chrono::microseconds>(end_time - start_time);
            trip_times[trips] = span.count();
            type_send = (side == SERVER) ? ReqType::RTTServer : ReqType::RTTClient;
            RTTPacket out(type_send, p_content, strlen(p_content)); /*Send opposite type of packet to other side*/
            res = out.Send();
            dprintm("[l] send", res)
            start_time = TIME_METHOD::now();
        }
    }
    if (side == CLIENT) {
        RTTPacket last_one(ReqType::RTTServer, NO_CONTENT, 1);
        res = last_one.Receive();
        dprintm("Getting the the last one", res)
    }
    int average = 0, r_total = 0;
    for (int i = 0; i < 5; i++)
        r_total += trip_times[i];
    average = r_total / 5;
    rtt_determined.tv_usec = average * 2;
    rtt_determined.tv_sec = 0;
    cout << "Round trip time = " << average << "microsec" << endl;
    use_manual_timeout = false;
    ResetTimeout(0, 0);
    return average;
}

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
 * Blocks until a packet is received or Timeout.
 *
 * @param *packet_buf raw packet buffer received
 * @param buff_len length of packet buffer
 * @param &type Single letter string representing type of packet
 *
 * @return result of awaiting.
 */
StatusResult Sockets::AwaitPacket(char *packet_buf, size_t *buff_len, string &type) {
    if (!this->initialized) return StatusResult::NotInitialized;
    StatusResult rec = ReceiveTimeout(packet_buf, buff_len);
    if (rec != StatusResult::Success) return rec;
    DataPacket *temp = new DataPacket();
    temp->packet_size = *buff_len;
    memcpy(temp->packet_buffer, packet_buf, *buff_len);
    *buff_len = *buff_len;
    temp->ConvertFromBuffer();
    type.clear();
    type.insert(0, temp->type_string, 1);
    return StatusResult::Success;
}

