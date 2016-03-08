//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================

#include "packets.h"

Packet::Packet(int from_port, int to_port, string to_ipadr) :
		Packet(from_port, to_port, to_ipadr, NULL) {
}

Packet::Packet(int from_port, int to_port, string to_ipadr, const char* data) :
		content(data), content_length(-1), to_ip_address(to_ipadr), from_port(
				from_port), to_port(to_port), packet_size(0), checksum(0), type_string(
				"XXX") {
	memset(packet_buffer, NOTHING, PACKET_SIZE);
}

Packet::~Packet() {
	// TODO Auto-generated destructor stub
}

DecodeResult Packet::DecodePacket(void* buff, size_t buffsize, Packet* out) {

	return DecodeResult::FatalError;
}

uint16_t Packet::Checksum() {
	register uint16_t sum;
	unsigned short oddbyte;

	sum = 0;
	while (packet_size > 1) {
		sum += (*packet_buffer)++;
		packet_size -= 2;
	}
	if (packet_size == 1) {
		oddbyte = 0;
		*((u_char*) &oddbyte) = *(u_char*) packet_buffer;
		sum += oddbyte;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum = sum + (sum >> 16);
	sum = (short) ~sum;
	return sum;
}

void Packet::Finalize() {
	char *packet_type = (char *) packet_buffer;
	uint16_t *checksum = (uint16_t *) (packet_buffer + sizeof(char) * 3);
	char *data = (char*) (packet_buffer + sizeof(uint16_t) + sizeof(char) * 3);
	strcpy(packet_type, type_string);
	*checksum = 50000;
	strcpy(data, content);
}

void Packet::Send() {

}

void Packet::_send_to_socket() {

}

DataPacket::DataPacket(int from_port, int to_port, string to_ipadr,
		const char* data) :
		Packet(from_port, to_port, to_ipadr, data) {
	memset(packet_buffer, NOTHING, PACKET_SIZE);
	type_string = DATA;
}

AckPacket::AckPacket() :
		Packet(0, 0, "") {
	memset(packet_buffer, NOTHING, PACKET_SIZE);
	type_string = ACK;
}

NakPacket::NakPacket() :
		Packet(0, 0, "") {
	memset(packet_buffer, NOTHING, PACKET_SIZE);
	type_string = NO_ACK;
}

RequestPacket::RequestPacket(ReqType type) :
		Packet(0, 0, "") {
	memset(packet_buffer, NOTHING, PACKET_SIZE);
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
	}
}

