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
    side = SERVER;
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
    side = CLIENT;
    StatusResult bind_res = BindAddresses(address_from, address_to, port_from, port_to);
    if (bind_res != StatusResult::Success)
        return bind_res;

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
    socklen_t length = sizeof(client_sock_addr); /*length of struct*/
    size_t res = (size_t) recvfrom(socket_id, buffer, *bufflen, 0, (sockaddr *) &client_sock_addr, &length);
    if (res < 0) { /*If error occurred*/
        perror("Error on receive");
        return StatusResult::FatalError;
    }
    buffer[res] = 0; /*Set last byte to null*/
    if (res > PACKET_SIZE) {
        cerr << "PACKET TO LARGE (" << res << "), DROPPED" << endl;
        return StatusResult::FatalError;
    }
    *bufflen = res; /*Return length via pointer*/
    if (DEBUG) {
        cout << "recvd packet [begin]";
        fwrite(buffer, *bufflen, 1, stdout);
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
StatusResult Sockets::ReceiveTimeout(char *buffer, size_t *bufflen, timeval &timeout) {
    FD_ZERO (&socks);
    FD_SET(socket_id, &socks);
    struct timeval *temp = new timeval;
    memcpy(temp, &timeout, sizeof(timeval));
    int rval = select(socket_id + 1, &socks, NULL, NULL, temp);
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
        fwrite(buffer, *bufflen, 1, stdout);
        cout << " " << "[end]" << endl;
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
    chrono::steady_clock::time_point start_time, end_time;
    if (side == CLIENT) {
        dprint("RTTT side", "client")
        RTTPacket client_send(ReqType::RTTClient, dat, strlen(dat));
        RTTPacket server_resp(ReqType::RTTServer, dat, strlen(dat));
        dprintm("[c] to server", client_send.Send())
        dprintm("[c] rcv server", res = server_resp.Receive())
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
        dprintm("[s] rcv client", res = from_client.Receive())
        if (res == StatusResult::Timeout) {
            perror("Client did not respond to RTT start");
            return -1;
        } else if (res == StatusResult::Success) {
            RTTPacket my_resp(ReqType::RTTServer);
            dprintm("[s] to client", my_resp.Send())
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
            dprintm("[li] send", init.Send())
            start_time = chrono::steady_clock::now();
        }
        RTTPacket in(type_send, p_content, strlen(p_content));
        dprintm("[l] rcv", res = in.Receive())
        if (res == StatusResult::Timeout) { /*Select timed out*/
            perror("SELECT timeout");
            trip_times[trips] = 0;
        } else { /*the socket is ready to be read from*/
            end_time = chrono::steady_clock::now();
            chrono::microseconds span = chrono::duration_cast<chrono::microseconds>(end_time - start_time);
            trip_times[trips] = span.count();
            type_send = (side == SERVER) ? ReqType::RTTServer : ReqType::RTTClient;
            RTTPacket out(type_send, p_content, strlen(p_content)); /*Send opposite type of packet to other side*/
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
    use_manual_timeout = false;
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
 * @param packet Packet to be returned by pointer
 * @param type String representing packet returned
 *
 * @return result of awaiting.
 */
StatusResult Sockets::AwaitPacket(class Packet *packet, string &type) {
    if (!this->initialized) return StatusResult::NotInitialized;
    char buffer[PACKET_SIZE];
    size_t length = PACKET_SIZE;
    StatusResult rec = ReceiveTimeout(buffer, &length);
    if (rec != StatusResult::Success) return rec;
    DataPacket *temp = new DataPacket();
    memcpy(temp->packet_buffer, buffer, length);
    temp->packet_size = length;
    temp->ConvertFromBuffer();
    dprintm("Await init dec res", temp->DecodePacket())
    switch (temp->type_string[0]) {
        case 'A': //ACK
            packet = new AckPacket(temp->sequence_num);
            type = ACK;
            return packet->DecodePacket(buffer, length);
        case 'N': //NO_ACK
            packet = new NakPacket(temp->sequence_num);
            type = NO_ACK;
            return packet->DecodePacket(buffer, length);
        case 'G': //GET_INFO
            packet = new RequestPacket(ReqType::Info, temp->content, strlen(temp->content));
            type = GET_INFO;
            return packet->DecodePacket(buffer, length);
        case 'F': //GET_FAIL
            packet = new RequestPacket(ReqType::Fail, temp->content, strlen(temp->content));
            type = GET_FAIL;
            return packet->DecodePacket(buffer, length);
        case 'S': //GET_SUCCESS
            packet = new RequestPacket(ReqType::Success, temp->content, strlen(temp->content));
            type = GET_SUCCESS;
            return packet->DecodePacket(buffer, length);
        case 'C': //RTT_TEST_CLIENT
            packet = new RTTPacket(ReqType::RTTClient);
            type = RTT_TEST_CLIENT;
            return packet->DecodePacket(buffer, length);
        case 'V': //RTT_TEST_SERVER
            packet = new RTTPacket(ReqType::RTTServer);
            type = RTT_TEST_SERVER;
            return packet->DecodePacket(buffer, length);
        case 'D': //DATA
            packet = new DataPacket(temp->content, temp->content_length);
            type = DATA;
            return packet->DecodePacket(buffer, length);
        case 'H': //GREETING
            packet = new GreetingPacket(temp->content, temp->content_length);
            type = GREETING;
            return packet->DecodePacket(buffer, length);
        default:
            return StatusResult::FatalError;
    }
}

StatusResult Sockets::AwaitPacket(char *packet_buf, size_t *buff_len, string &type) {
    if (!this->initialized) return StatusResult::NotInitialized;
    char buffer[PACKET_SIZE];
    size_t length = PACKET_SIZE;
    StatusResult rec = ReceiveTimeout(buffer, &length);
    if (rec != StatusResult::Success) return rec;
    DataPacket *temp = new DataPacket();
    memcpy(packet_buf, buffer, length);
    *buff_len = length;
    //temp->ConvertFromBuffer();
    //dprintm("Await init dec res", temp->DecodePacket())
    return  StatusResult::Success;
}

