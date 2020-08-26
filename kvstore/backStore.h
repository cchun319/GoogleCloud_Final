/*
 * backStore.h
 *
 *  Created on: 2018年4月13日
 *      Author: BearBurg
 */

#ifndef BACKSTORE_H_
#define BACKSTORE_H_

#include <string>
#include <vector>
#include <list>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Connection.h"

class BackStore {
	class Replicate {
	public:
		sockaddr_in addr;
		//1-running 2-recovering 3-fail
		int state;
		std::list<std::string> holdBack;	
		Connection* connection;

		Replicate(sockaddr_in addr, Connection* connection) {
			this->addr = addr;
			state = 1;
			this->connection = connection;
		}

	} ;

	std::vector<Replicate*> nodes;
	int myID;
	std::vector<Connection*> connections;


public:
	BackStore(std::vector<struct sockaddr_in> list, int myID);

	bool establishConn(std::vector<struct sockaddr_in> list);

	bool push(std::string row, std::string col, std::string* data, long seq);

	bool poll(std::string row, std::string col, long seq);

	bool reconnect(std::string address);

	bool recover(std::string address);

};



#endif /* BACKSTORE_H_ */
