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
	bool socket_ready;
	bool initialized;
public:
	Sockets();
	StatusResult Open(string addresses, int port);
	static Sockets* instance() {
		if (manager == NULL) {
			manager = new Sockets();
		}
		return manager;
	}
	StatusResult Send(void *buff, size_t length);
	virtual ~Sockets();
};

#endif /* SOCKETS_H_ */
