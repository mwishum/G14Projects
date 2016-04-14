//============================================================================
// Project 2: Reliable FTP Using GBN over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// April 15, 2016
//============================================================================

#ifndef PACKETS_H_
#define PACKETS_H_

#include "project.h"
#include "Sockets.h"

#define ACK               "A"
#define NO_ACK            "N"
#define GET_INFO          "G"
#define GET_FAIL          "F"
#define GET_SUCCESS       "S"
#define DATA              "D"
#define RTT_TEST_CLIENT   "C"
#define RTT_TEST_SERVER   "V"
#define GREETING          "H"

using namespace std;
/**
 * Type used for RTTPacket and RequestPacket
 */
enum class ReqType {
    Info, Fail, Success, RTTClient, RTTServer
};

class Packet {
    friend class Sockets;

public:
    Packet();
    virtual ~Packet();
    virtual StatusResult DecodePacket();
    virtual StatusResult DecodePacket(char *packet_buffer, size_t buf_length);
    virtual void ConvertFromBuffer();
    virtual uint16_t Checksum();
    virtual void Finalize();
    virtual StatusResult Send();
    virtual StatusResult Receive();
    static size_t max_content();
    char *Content() {
        return content;
    }
    size_t ContentSize() {
        return content_length;
    }
    void Sequence(uint8_t n_seq);
    uint8_t Sequence();

protected:
    virtual StatusResult _send_to_socket();
    Packet(char *data, size_t data_len);
    char *content;
    size_t content_length;
    char packet_buffer[PACKET_SIZE];
    size_t packet_size;
    uint16_t checksum;
    uint8_t sequence_num;
    const char *type_string;
};

class DataPacket : public Packet {
public:
    DataPacket(char *data, size_t data_len);
    DataPacket();
};

class AckPacket : public DataPacket {
public:
    AckPacket(uint8_t seq);
    StatusResult Send() {
        Finalize();
        return _send_to_socket();
    }
};

class NakPacket : public DataPacket {
public:
    NakPacket(uint8_t seq);
    StatusResult Send() {
        Finalize();
        return _send_to_socket();
    }
};

class RequestPacket : public DataPacket {
public:
    RequestPacket(ReqType type, char *data, size_t data_len);
};

class RTTPacket : public DataPacket {
public:
    RTTPacket(ReqType type, char *data, size_t data_len);
    RTTPacket(ReqType type);
    StatusResult Send() {
        Finalize();
        return _send_to_socket();
    }
};

class GreetingPacket : public DataPacket {
public:
    GreetingPacket(char *data, size_t data_len);
};

#endif /* PACKETS_H_ */

