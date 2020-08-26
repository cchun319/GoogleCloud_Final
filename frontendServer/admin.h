/*
 * admin.h
 *
 *  Created on: Apr 27, 2018
 *      Author: cis505
 */

#ifndef ADMIN_H_
#define ADMIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <pthread.h>
#include <deque>
#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include "request.h"
#include "response.h"
#include "util.h"

using namespace std;

// wrapper class for the server
class Server {
public:
	string address;
	int port;
	bool running;
	Server(string address, int port) :
			address(address), port(port), running(false) {};
};


#endif /* ADMIN_H_ */
