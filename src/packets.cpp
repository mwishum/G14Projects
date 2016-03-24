//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================

#include "packets.h"
#include "Sockets.h"
#include "Gremlin.h"

Packet::Packet() :
        content(NULL), content_length(1), packet_size(0), checksum(0), sequence_num(
        0), type_string("X") {
    content = new char[1];
    //memcpy(content, NO_CONTENT, 1);
    memset(packet_buffer, NOTHING, PACKET_SIZE);
}

Packet::Packet(char *data, size_t data_len) :
        content(NULL), content_length(data_len), packet_size(0), checksum(0), sequence_num(
        0), type_string("X") {
    content = new char[data_len + 1];
    memcpy(content, data, data_len);
    memset(packet_buffer, NOTHING, PACKET_SIZE);
}

Packet::~Packet() {
    //delete[] content;
}

StatusResult Packet::DecodePacket() {
    char *buf_pack_type = (char *) packet_buffer;
    uint8_t *seq_num = (uint8_t *) (packet_buffer + sizeof(char));
    uint16_t *buf_checksum = (uint16_t *) (packet_buffer + sizeof(uint8_t)
                                           + sizeof(char));
    char *data = (char *) (packet_buffer + sizeof(uint8_t) + sizeof(uint16_t)
                           + sizeof(char));

    bool type_matches = *buf_pack_type == *type_string;
    this->checksum = *buf_checksum;
    *buf_checksum = 0;
    uint16_t actual_sum = Checksum();
    dprintcmp(" TYPE", buf_pack_type, this->type_string)
    dprintcmph(" CHKSUM", this->checksum, actual_sum);
    dprintcmph(" SEQNUM", (uint) *seq_num, (uint) this->sequence_num);
    if (this->checksum != actual_sum) {
        //Packet is invalid
        return StatusResult::ChecksumDoesNotMatch;
    }
    assert(this->checksum == actual_sum);
    assert(content != NULL);
    assert(data != NULL);

    content_length = strlen(data);
    content = new char[content_length];
    strcpy(content, data);
    packet_size = sizeof(char) + sizeof(uint16_t) + content_length;

    if (this->sequence_num != *seq_num) {
        //Packet was not expected
        return StatusResult::OutOfSequence;
    } else {
        this->sequence_num = *seq_num;
    }
    if (!type_matches) {
        return StatusResult::NotExpectedType;
    }
    return StatusResult::Success;
}

uint16_t Packet::Checksum() {
    register uint16_t sum;
    uint8_t oddbyte;
    uint16_t *pointer = (uint16_t *) packet_buffer;
    size_t size = packet_size;
    sum = 0;
    while (size > 1) {
        sum += *pointer;
        pointer++;
        size -= 2;
    }
    if (size == 1) {
        oddbyte = 0;
        *(&oddbyte) = *(u_char *) packet_buffer;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);
    return ~sum;
}

size_t Packet::max_content() {
    return PACKET_SIZE - (sizeof(uint8_t) + sizeof(uint16_t) + sizeof(char));
}

void Packet::Finalize() {
    char *packet_type = (char *) packet_buffer;
    uint8_t *seq_num = (uint8_t *) (packet_buffer + sizeof(char));
    uint16_t *checksum = (uint16_t *) (packet_buffer + sizeof(uint8_t)
                                       + sizeof(char));
    char *data = (char *) (packet_buffer + sizeof(uint8_t) + sizeof(uint16_t)
                           + sizeof(char));
    strcpy(packet_type, type_string);
    *checksum = 0;
    *seq_num = sequence_num;

    assert(content_length <= max_content());
    assert(content != NULL);

    strcpy(data, content);
    packet_size = sizeof(char) + sizeof(uint16_t) + sizeof(uint8_t)
                  + content_length;
    this->checksum = Checksum();
    *checksum = this->checksum;
    //printBinary(*checksum);
}

StatusResult Packet::Send() {
    Finalize();

    StatusResult r = StatusResult::Success;
    if (Sockets::instance()->GetSide() == SERVER
        && Gremlin::instance()->tamper(packet_buffer, &packet_size) == StatusResult::Success) {
        r = _send_to_socket();
    } else if (Sockets::instance()->GetSide() == CLIENT) {
        r = _send_to_socket();
    }

    return r;
}

StatusResult Packet::_send_to_socket() {
    //CALL FINALIZE BEFORE
    return Sockets::instance()->Send(packet_buffer, &packet_size);
}

StatusResult Packet::Receive() {
    packet_size = PACKET_SIZE;
    StatusResult res = Sockets::instance()->ReceiveTimeout(packet_buffer,
                                                           &packet_size);
    if (res != StatusResult::Success) {
        return res;
    } else
        return DecodePacket();

}

void Packet::Sequence(uint8_t n_seq) {
    sequence_num = n_seq;
}

uint8_t Packet::Sequence() {
    return sequence_num;
}

StatusResult Packet::DecodePacket(char *packet_buffer, size_t buf_length) {
    memcpy(this->packet_buffer, packet_buffer, buf_length);
    this->packet_size = buf_length;
    ConvertFromBuffer();
    return DecodePacket();
}

/**
 * Converts this packet to the type contained in the buffer.
 *
 * WARNING: Only use when you know the buffer is of the same type as the packet
 */
void Packet::ConvertFromBuffer() {
    char *buf_pack_type = (char *) packet_buffer;
    uint8_t *seq_num = (uint8_t *) (packet_buffer + sizeof(char));
    char *data = (char *) (packet_buffer + sizeof(uint8_t) + sizeof(uint16_t)
                           + sizeof(char));

    type_string = buf_pack_type;
    content_length = packet_size - ((size_t) -((int) max_content() - PACKET_SIZE));
    strcpy(content, data);
    this->sequence_num = *seq_num;
}


DataPacket::DataPacket(char *data, size_t data_len) :
        Packet(data, data_len) {
    type_string = DATA;
}

DataPacket::DataPacket() :
        Packet() {
    type_string = DATA;
}

AckPacket::AckPacket(uint8_t seq) :
        DataPacket() {
    type_string = ACK;
    Sequence(seq);
}

NakPacket::NakPacket(uint8_t seq) :
        DataPacket() {
    type_string = NO_ACK;
    Sequence(seq);
}

RequestPacket::RequestPacket(ReqType type, char *data, size_t data_len) :
        DataPacket(data, data_len) {
    switch (type) {
        case ReqType::Fail:
            type_string = GET_FAIL;
            break;
        case ReqType::Info:
            type_string = GET_INFO;
            break;
        case ReqType::Success:
            type_string = GET_SUCCESS;
            break;
        default:
            break;
    }
}

RTTPacket::RTTPacket(ReqType type, char *data, size_t data_len) :
        DataPacket(data, data_len) {
    if (type == ReqType::RTTClient) {
        type_string = RTT_TEST_CLIENT;
    } else {
        type_string = RTT_TEST_SERVER;
    }
}

RTTPacket::RTTPacket(ReqType type) :
        DataPacket(NO_CONTENT, 1) {
    if (type == ReqType::RTTClient) {
        type_string = RTT_TEST_CLIENT;
    } else {
        type_string = RTT_TEST_SERVER;
    }
}


GreetingPacket::GreetingPacket(char *data, size_t data_len) : DataPacket(data, data_len) {
    type_string = GREETING;
}

