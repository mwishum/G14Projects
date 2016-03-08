/*
 * packets.h
 *
 *  Created on: Mar 1, 2016
 *      Author: mwishum
 */

#ifndef PACKETS_H_
#define PACKETS_H_

#include "project.h"

using namespace std;

enum DecodeResult {
	Success,
	FatalError,
	ChecksumDoesNotMatch
};

class DataPacket {
public:
	DataPacket(int from_port, int to_port, string to_ipadr, string data);
	virtual ~DataPacket();
	static DecodeResult DecodePacket(void* buff, size_t buffsize, DataPacket* out);
	uint16_t Checksum();
	void Finalize();

private:
	string content;
	string to_ip_address;
	int from_port, to_port;
	char packet_buffer[PACKET_SIZE];
	int packet_size;
	uint16_t checksum;
};


#endif /* PACKETS_H_ */

