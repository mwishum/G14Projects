/*
 * packets.cpp
 *
 *  Created on: Mar 1, 2016
 *      Author: mwishum
 */

#include "packets.h"

DataPacket::DataPacket(int from_port, int to_port, string to_ipadr, string data) :
		content(data), to_ip_address(to_ipadr), from_port(from_port), to_port(
				to_port), packet_size(0), checksum(0) {
	memset(packet_buffer, NOTHING, PACKET_SIZE);
}

DataPacket::~DataPacket() {
	// TODO Auto-generated destructor stub
}

DecodeResult DataPacket::DecodePacket(void* buff, size_t buffsize,
		DataPacket* out) {

	return FatalError;
}

uint16_t DataPacket::Checksum() {
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

void DataPacket::Finalize() {
	uint16_t *ip_addr = (uint16_t *) packet_buffer;
	uint16_t *checksum = (uint16_t *) (packet_buffer + sizeof(uint16_t));
	char *data = (char*) (packet_buffer + sizeof(uint16_t) + sizeof(uint16_t));
	*ip_addr = 4;
	*checksum = 30000;
	strcpy(data, content.c_str());
}
