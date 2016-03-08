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

#define ACK 			"ACK"
#define NO_ACK 			"NAK"
#define GET_INFO 		"GET"
#define GET_FAIL 		"NOG"
#define GET_SUCCESS		"SUG"
#define GET_DONE_SUCCESS "SCP"
#define DATA			"DAT"
#define RTT_TEST_CLIENT	"RTC"
#define RTT_TEST_SERVER	"RTS"
#define GREETING		"GRT"

using namespace std;

enum class DecodeResult {
	Success, FatalError, ChecksumDoesNotMatch
};

enum class ReqType {
	Info, Fail, Success
};

class Packet {
public:
	Packet(int from_port, int to_port, string to_ipadr);
	virtual ~Packet();
	static DecodeResult DecodePacket(void* buff, size_t buffsize, Packet* out);
	virtual uint16_t Checksum() final;
	virtual void Finalize();
	virtual void Send();
	virtual void _send_to_socket();
	string GetContent() {
		return content;
	}

protected:
	Packet(int from_port, int to_port, string to_ipadr, const char* data);
	const char* content;
	size_t content_length;
	string to_ip_address;
	int from_port, to_port;
	char packet_buffer[PACKET_SIZE];
	int packet_size;
	uint16_t checksum;
	const char* type_string;
};

class DataPacket: public Packet {
public:
	DataPacket(int from_port, int to_port, string to_ipadr, const char* data);
};

class AckPacket: public Packet {
public:
	AckPacket();
};

class NakPacket: public Packet {
public:
	NakPacket();
};

class RequestPacket: public Packet {
public:
	RequestPacket(ReqType type);
};

#endif /* PACKETS_H_ */

