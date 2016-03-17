/*
 * Sockets.cpp
 *
 *  Created on: Mar 9, 2016
 *      Author: mwishum
 */

#include "Sockets.h"

Sockets::Sockets() {
	// TODO Auto-generated constructor stub

}

StatusResult Sockets::Open(string addresses, int port) {
	socket_id = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_id < 0) {
		return StatusResult::CouldNotOpen;
	}

	return StatusResult::NotInitialized;
}

void Sockets::Close() {
}

StatusResult Sockets::Receive(void* buffer, size_t* bufflen) {
	return StatusResult::NotInitialized;
}

StatusResult Sockets::Send(void* buff, size_t* length) {
	return StatusResult::NotInitialized;
}

Sockets::~Sockets() {
	// TODO Auto-generated destructor stub
}

