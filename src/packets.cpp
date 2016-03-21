//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================

#include "packets.h"
#include "Sockets.h"

Packet::Packet() :
		Packet(NO_CONTENT) {
}

Packet::Packet(char* data) :
		content(data), content_length(strlen(data)), packet_size(0), checksum(0), type_string("XXX") {
	memset(packet_buffer, NOTHING, PACKET_SIZE);
}

Packet::~Packet() {
	//free (content);
}

StatusResult Packet::DecodePacket() {
	char *buf_pack_type = (char *) packet_buffer;
	uint16_t *buf_checksum = (uint16_t *) (packet_buffer + sizeof(char) * 3);
	char *data = (char*) (packet_buffer + sizeof(uint16_t) + sizeof(char) * 3);

	string buf_pack_type_s;
	buf_pack_type_s.insert(0, buf_pack_type, 3);
	bool type_matches = buf_pack_type_s == type_string;
	this->checksum = *buf_checksum;
	*buf_checksum = 0;
	uint16_t actualsum = Checksum();
	dprint("pck type", buf_pack_type_s)
	dprint("act type", type_string)
	assert(type_matches);
	dprint("pck checksum", this->checksum);
	dprint("act checksum", actualsum);
	assert(this->checksum == actualsum);
	if (!type_matches) {
		return StatusResult::NotExpectedType;
	}
	if (this->checksum != actualsum) {
		return StatusResult::ChecksumDoesNotMatch;
	}
	content_length = strlen(data);
	content = (char*) malloc(content_length);
	strcpy(content, data);
	packet_size = sizeof(char) * 3 + sizeof(uint16_t) + content_length;

	return StatusResult::Success;
}

uint16_t Packet::Checksum() {
	register uint16_t sum;
	uint8_t oddbyte;
	uint16_t *pointer = (uint16_t*) packet_buffer;
	int size = packet_size;
	sum = 0;
	while (size > 1) {
		sum += *pointer;
		pointer++;
		size -= 2;
	}
	if (size == 1) {
		oddbyte = 0;
		*((u_char*) &oddbyte) = *(u_char*) packet_buffer;
		sum += oddbyte;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum = sum + (sum >> 16);
	return ~sum;
}

void Packet::Finalize() {
	char *packet_type = (char *) packet_buffer;
	uint16_t *checksum = (uint16_t *) (packet_buffer + sizeof(char) * 3);
	char *data = (char*) (packet_buffer + sizeof(uint16_t) + sizeof(char) * 3);
	strcpy(packet_type, type_string);
	*checksum = 0;

	strcpy(data, content);
	packet_size = sizeof(char) * 3 + sizeof(uint16_t) + content_length;
	*checksum = Checksum();

	//Received packed from server
	//cout << "packet checksum: ";
	//printBinary(*checksum);
	//uint16_t secondsum = Checksum();
	//cout << "Re-checksumed (with existing csum):" << endl;
	//printBinary(secondsum);
	//*checksum = 0;
	//packet_size = sizeof(char) * 3 + sizeof(uint16_t) + content_length;
	//uint16_t zerosum = Checksum();
	//cout << "Re-checksumed (after zeroing csum):" << endl;
	//printBinary(zerosum);
	//uint16_t added = zerosum + ~secondsum;
	//cout << "zeroed csum + existing csum:" << endl;
	//printBinary(added);
}

StatusResult Packet::Send() {
	Finalize();

	StatusResult r = StatusResult::Success;
	//Only tamper with buffer when still sending packet
	//with errors added.
	//if(gremlin(packet_buffer, packet_size)) {
	r = _send_to_socket();
	//}
	return r;
}

StatusResult Packet::_send_to_socket() {
	return Sockets::instance()->Send(packet_buffer, &packet_size);
}

StatusResult Packet::Receive() {
	packet_size = PACKET_SIZE;
	StatusResult res = Sockets::instance()->Receive(packet_buffer, &packet_size);
	if (res != StatusResult::Success) {
		return res;
	} else
		return DecodePacket();

}

DataPacket::DataPacket(char* data) :
		Packet(data) {
	type_string = DATA;
}

DataPacket::DataPacket() :
		Packet() {
	type_string = DATA;
}

AckPacket::AckPacket() :
		DataPacket() {
	type_string = ACK;
}

NakPacket::NakPacket() :
		DataPacket() {
	type_string = NO_ACK;
}

RequestPacket::RequestPacket(ReqType type) :
		DataPacket() {
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

RTTPacket::RTTPacket(ReqType type, char* data) :
		DataPacket(data) {
	if (type == ReqType::RTTClient) {
		type_string = RTT_TEST_CLIENT;
	} else {
		type_string = RTT_TEST_SERVER;
	}
}

RTTPacket::RTTPacket(ReqType type) :
		RTTPacket(type, NO_CONTENT) {
}

