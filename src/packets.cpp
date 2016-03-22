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
		Packet(NO_CONTENT,1) {
		}

Packet::Packet(char* data, size_t data_len) :
		content(NULL), content_length(data_len), packet_size(0), checksum(0), sequence_num(
				0), type_string("X") {
	content = new char[data_len];
	memcpy(content, data, data_len);
	memset(packet_buffer, NOTHING, PACKET_SIZE);
}

Packet::~Packet() {
	//delete[] content;
}

StatusResult Packet::DecodePacket() {
	char *buf_pack_type = (char *) packet_buffer;
	uint8_t *seq_num = (uint8_t*) packet_buffer + sizeof(char);
	uint16_t *buf_checksum = (uint16_t *) (packet_buffer + sizeof(uint8_t)
			+ sizeof(char));
	char *data = (char*) (packet_buffer + sizeof(uint8_t) + sizeof(uint16_t)
			+ sizeof(char));

	bool type_matches = *buf_pack_type == *type_string;
	this->checksum = *buf_checksum;
	*buf_checksum = 0;
	uint16_t actualsum = Checksum();
	dprint("pck type", buf_pack_type)
	dprint("act type", type_string)
	dprint("pck checksum", this->checksum);
	dprint("act checksum", actualsum);
	dprint("pck seq#", *seq_num);
	dprint("act seq#", sequence_num);
	if (!type_matches) {
		return StatusResult::NotExpectedType;
	}
	if (this->checksum != actualsum) {
		return StatusResult::ChecksumDoesNotMatch;
	}
	if (this->sequence_num != *seq_num) {
		return StatusResult::OutOfSequence;
	}
	assert(this->checksum == actualsum);
	assert(type_matches);
	assert(content != NULL);
	assert(data != NULL);
	content_length = strlen(data);
	content = new char[content_length];
	strcpy(content, data);
	packet_size = sizeof(char) + sizeof(uint16_t) + content_length;

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

size_t Packet::max_content() {
	return 256 - (sizeof(uint8_t) + sizeof(uint16_t) + sizeof(char));
}

void Packet::Finalize() {
	char *packet_type = (char *) packet_buffer;
	uint8_t *seq_num = (uint8_t*) packet_buffer + sizeof(char);
	uint16_t *checksum = (uint16_t *) (packet_buffer + sizeof(uint8_t)
			+ sizeof(char));
	char *data = (char*) (packet_buffer + sizeof(uint8_t) + sizeof(uint16_t)
			+ sizeof(char));
	strcpy(packet_type, type_string);
	*checksum = 0;
	*seq_num = sequence_num;

	if (content_length > max_content())
		dprint("ERROR", "content too big")
	strcpy(data, content);
	packet_size = sizeof(char) + sizeof(uint16_t) + sizeof(uint8_t)
			+ content_length;
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
	//CALL FINALIZE ON YOUR OWN
	return Sockets::instance()->Send(packet_buffer, &packet_size);
}

StatusResult Packet::Receive() {
	packet_size = PACKET_SIZE;
	StatusResult res = Sockets::instance()->Receive(packet_buffer,
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

DataPacket::DataPacket(char* data, size_t data_len) :
		Packet(data, data_len) {
	type_string = DATA;
}

DataPacket::DataPacket() :
		Packet() {
	type_string = DATA;
}

AckPacket::AckPacket(uint8_t seq) :
		DataPacket(NO_CONTENT,seq) {
			type_string = ACK;
		}

NakPacket::NakPacket(uint8_t seq) :
		DataPacket(NO_CONTENT,seq) {
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
		DataPacket(data, 0) {
	if (type == ReqType::RTTClient) {
		type_string = RTT_TEST_CLIENT;
	} else {
		type_string = RTT_TEST_SERVER;
	}
}

RTTPacket::RTTPacket(ReqType type) :
		RTTPacket(type, NO_CONTENT) {
		}

