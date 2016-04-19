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
#include "chrono"

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

/**
 * Class Packet
 *
 * A Packet is a representation of the data to be sent and its required
 * information. It contains basic methods but can contain no content
 * (use DataPacket).
 *
 * Send, SendDelayed, Receive, Content, ContentSize, and Sequence are
 * expected to be called, and will be set in subclasses.
 *
 */
class Packet {
    friend class Sockets;

public:
    Packet();
    virtual ~Packet();
    virtual SR DecodePacket();
    virtual SR DecodePacket(char *packet_buffer, size_t buf_length);
    virtual void ConvertFromBuffer();
    virtual uint16_t Checksum();
    virtual void Finalize();
    virtual SR Send();
    virtual SR SendDelayed();
    virtual SR Receive();
    static size_t header_size();
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
    virtual SR send_delayed(chrono::milliseconds time, Packet p);
    virtual SR _send_to_socket();
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
    SR Send() {
        Finalize();
        return _send_to_socket();
    }
};

class NakPacket : public DataPacket {
public:
    NakPacket(uint8_t seq);
    SR Send() {
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
    SR Send() {
        Finalize();
        return _send_to_socket();
    }
};

class GreetingPacket : public DataPacket {
public:
    GreetingPacket(char *data, size_t data_len);
};

class UnknownPacket : public DataPacket {
    friend class Sockets;

public:
    UnknownPacket();

private:
    UnknownPacket(char *packet_buffer, size_t buf_length);

};

#endif /* PACKETS_H_ */

