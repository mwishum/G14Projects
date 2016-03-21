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
#define DATA			"DAT"
#define RTT_TEST_CLIENT	"RTC"
#define RTT_TEST_SERVER	"RTS"
#define GREETING		"GRT"

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
	string GetContent() {
		return content;
	}
protected:
	Packet(char* data);
	char* content;
	size_t content_length;
	char packet_buffer[PACKET_SIZE];
	size_t packet_size;
	uint16_t checksum;
	const char* type_string;
};

class DataPacket: public Packet {
public:
	DataPacket(char* data);
	DataPacket();
};

class AckPacket: public DataPacket {
public:
	AckPacket();
};

class NakPacket: public DataPacket {
public:
	NakPacket();
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

