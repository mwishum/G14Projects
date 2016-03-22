//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================

#ifndef PACKETS_H_
#define PACKETS_H_

#include "project.h"

#define ACK 			"A"
#define NO_ACK 			"N"
#define GET_INFO 		"G"
#define GET_FAIL 		"F"
#define GET_SUCCESS		"S"
#define DATA			"D"
#define RTT_TEST_CLIENT	"C"
#define RTT_TEST_SERVER	"V"
#define GREETING		"G"

using namespace std;

enum class ReqType {
	Info, Fail, Success, RTTClient, RTTServer
};

class Packet {
public:
	Packet();
	virtual ~Packet();
	virtual StatusResult DecodePacket();
	virtual uint16_t Checksum() final;
	virtual void Finalize();
	virtual StatusResult Send();
	virtual StatusResult Receive();
	virtual StatusResult _send_to_socket();
	static size_t max_content();
	char * Content() {
		return content;
	}
	size_t ContentSize() {
		return content_length;
	}
	void Sequence(uint8_t n_seq);
	uint8_t Sequence();

protected:
	Packet(char* data, size_t data_len);
	char* content;
	size_t content_length;
	char packet_buffer[PACKET_SIZE];
	size_t packet_size;
	uint16_t checksum;
	uint8_t sequence_num;
	const char* type_string;
};

class DataPacket: public Packet {
public:
	DataPacket(char* data, size_t data_len);
	DataPacket();
};

class AckPacket: public DataPacket {
public:
	AckPacket(uint8_t seq);
};

class NakPacket: public DataPacket {
public:
	NakPacket(uint8_t seq);
};

class RequestPacket: public DataPacket {
public:
	RequestPacket(ReqType type);
};

class RTTPacket: public DataPacket {
public:
	RTTPacket(ReqType type, char* data);
	RTTPacket(ReqType type);
};

#endif /* PACKETS_H_ */

