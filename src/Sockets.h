/*
 * Sockets.h
 *
 *  Created on: Mar 9, 2016
 *      Author: mwishum
 */

#ifndef SOCKETS_H_
#define SOCKETS_H_

#include "project.h"

using namespace std;

enum class StatusResult {
	Success, Error, NotInitialized, CouldNotOpen
};

class Sockets {
	static Sockets *manager;
	int socket_id;
	struct sockaddr_in client_addr;

	bool socket_ready;
	bool initialized;
public:
	Sockets();
	virtual ~Sockets();
	StatusResult Open(string addresses, int port);
	void Close();
	StatusResult Receive(void *buffer, size_t* bufflen);
	StatusResult Send(void *buffer, size_t* bufflen);
	static Sockets* instance() {
		if (manager == NULL) {
			manager = new Sockets();
		}
		return manager;
	}
};

#endif /* SOCKETS_H_ */
