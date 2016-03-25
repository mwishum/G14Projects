//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================

#include "packets.h"
#include "Gremlin.h"

Packet::Packet() :
        content(NULL), content_length(1), packet_size(0), checksum(0), sequence_num(
        0), type_string("X") {
    content = new char[1];
    memset(packet_buffer, NOTHING, PACKET_SIZE);
}

/**
 * Constructor for basic Packet
 *
 * @param data content for packet
 * @param data_len length of content (NOT strlen unless null-term)
 */
Packet::Packet(char *data, size_t data_len) :
        content(NULL), content_length(data_len), packet_size(0), checksum(0), sequence_num(
        0), type_string("X") {
    content = new char[data_len + 1];
    memcpy(content, data, data_len);
    memset(packet_buffer, NOTHING, PACKET_SIZE);
}

Packet::~Packet() {
}

/**
 * Sets this packet's data to what in the packet buffer
 * Used after calling Receive.
 *
 */
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
        printf("Packet has CKSUM err - ActSum:%#06x RecdSum:%#06x ", actual_sum, this->checksum);
        return StatusResult::ChecksumDoesNotMatch;
    }
    assert(this->checksum == actual_sum);
    assert(content != NULL);
    assert(data != NULL);

    content_length = packet_size - ((size_t) -((int) max_content() - PACKET_SIZE));
    content = new char[content_length];
    strcpy(content, data);
    packet_size = sizeof(char) + sizeof(uint16_t) + content_length;

    if (this->sequence_num != *seq_num) {
        //Packet was not expected
        printf("Packet mis-sequenced - ActSum:%#06x RecdSum:%#06x ", actual_sum, this->checksum);
        return StatusResult::OutOfSequence;
    } else {
        this->sequence_num = *seq_num;
    }
    if (!type_matches) {
        return StatusResult::NotExpectedType;
    }
    return StatusResult::Success;
}

/**
 * Calculates the checksum of the packet buffer and returns it
 * (the actual checksum value in the buffer MUST be zero)
 *
 * @return checksum value
 *
 */
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

/**
 * Returns the maximum size content can be in this packet.
 *
 */
size_t Packet::max_content() {
    return PACKET_SIZE - (sizeof(uint8_t) + sizeof(uint16_t) + sizeof(char));
}

/**
 * Create packet buffer from packet contents.
 *
 * MUST be called before sending.
 *
 */
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
}

/**
 * Send packet to far client.
 * (Gremlin may tamper with it)
 *
 * @returns status of sending packet
 */
StatusResult Packet::Send() {
    Finalize();

    StatusResult r = StatusResult::FatalError;
    if (Sockets::instance()->GetSide() == SERVER
        && Gremlin::instance()->tamper(packet_buffer, &packet_size) == StatusResult::Success) {
        r = _send_to_socket();
    } else if (Sockets::instance()->GetSide() == CLIENT) {
        r = _send_to_socket();
    }
    return r;
}

/**
 * Actually send the packet to socket
 * (Not effected by Gremlin)
 *
 * @return status of sending to socket.
 *
 */
StatusResult Packet::_send_to_socket() {
    //CALL FINALIZE BEFORE
    return Sockets::instance()->Send(packet_buffer, &packet_size);
}

/**
 * Receives a packet from far client and Decodes the packet
 *
 * @return status of receiving (if failure) OR Decoding if successfully received
 *
 */
StatusResult Packet::Receive() {
    packet_size = PACKET_SIZE;
    StatusResult res = Sockets::instance()->ReceiveTimeout(packet_buffer, &packet_size);
    if (res != StatusResult::Success) {
        return res;
    } else
        return DecodePacket();
}

/**
 * Sets sequence number of packet.
 *
 * @param n_seq Sequence nubmer to set
 */
void Packet::Sequence(uint8_t n_seq) {
    sequence_num = n_seq;
}

/**
 * Gets sequence number of packet.
 *
 * @return this packet's sequence number
 */
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

/**
 * Constructor for basic DataPacket
 *
 * @param data content for packet
 * @param data_len length of content (NOT strlen unless null-term)
 */
DataPacket::DataPacket(char *data, size_t data_len) :
        Packet(data, data_len) {
    type_string = DATA;
}

/**
 * Constructor for default DataPacket (no content)
 *
 */
DataPacket::DataPacket() :
        Packet() {
    type_string = DATA;
}

/**
 * Constructor for Acknowledgement packet
 *
 * @param seq sequence number of packet
 */
AckPacket::AckPacket(uint8_t seq) :
        DataPacket() {
    type_string = ACK;
    Sequence(seq);
}

/**
 * Constructor for Non-Acknowledgement packet
 *
 * @param seq sequence number of packet
 */
NakPacket::NakPacket(uint8_t seq) :
        DataPacket() {
    type_string = NO_ACK;
    Sequence(seq);
}

/**
 * Constructor for basic Request Packet
 *
 * @param type type of packet (Success, Fail, Info)
 * @param data content for packet
 * @param data_len length of content (NOT strlen unless null-term)
 */
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

/**
 * Constructor for Round Trip Test Packet
 *
 * @param type type of packet (RTTClient or RTTServer)
 * @param data content for packet
 * @param data_len length of content (NOT strlen unless null-term)
 */
RTTPacket::RTTPacket(ReqType type, char *data, size_t data_len) :
        DataPacket(data, data_len) {
    if (type == ReqType::RTTClient) {
        type_string = RTT_TEST_CLIENT;
    } else {
        type_string = RTT_TEST_SERVER;
    }
}

/**
 * Constructor for Round Trip Test Packet (no content)
 *
 * @param type type of packet (RTTClient or RTTServer)
 *
 */
RTTPacket::RTTPacket(ReqType type) :
        DataPacket(NO_CONTENT, 1) {
    if (type == ReqType::RTTClient) {
        type_string = RTT_TEST_CLIENT;
    } else {
        type_string = RTT_TEST_SERVER;
    }
}

/**
 * Constructor for Greeting Packet
 *
 * @param data content for packet
 * @param data_len length of content (NOT strlen unless null-term)
 */
GreetingPacket::GreetingPacket(char *data, size_t data_len) : DataPacket(data, data_len) {
    type_string = GREETING;
}

