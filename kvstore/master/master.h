/*
 * master.h
 *
 *  Created on: 2018年4月8日
 *      Author: BearBurg
 */

#ifndef MASTER_MASTER_H_
#define MASTER_MASTER_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>


#include <iostream>
#include <sstream>
#include <pthread.h>
#include <csignal>
#include <map>
#include <string>
#include <vector>

#include "../filetool.h"
#include "masterConnection.h"

class node {
public:
	std::vector<sockaddr_in> rep_addrs;
	sockaddr_in primary_addr;
	int nodeID;
	int row_num;
	bool running;
	masterConnection* conn;


	node(int ID) :
			nodeID(ID) {
		row_num = 0;
		running = false;

	}

	node(const node& other) {
		this->row_num = 0;
		this->nodeID = other.nodeID;
		this->primary_addr = other.primary_addr;
		this->rep_addrs = other.rep_addrs;
		this->running = false;
	}

	bool heartbeat() {
		if (!running) {
			if (!promoteNewPrimary()) {
				std::cout <<"node: " << nodeID << " lose conn" << std::endl;
			}
		} else {
			if (conn->do_write("ALIVE\r\n",7)) {
				//std::cout << "alive" << std::endl;
				return true;
			} else {
				std::cout <<"node: " << nodeID << " lose conn" << std::endl;
				delete conn;
				running = false;

				promoteNewPrimary();
				
			}
		}

		return true;
	}

	bool promoteNewPrimary() {
		int sockfd = socket(PF_INET, SOCK_STREAM, 0);
		for (int j = 0; j < rep_addrs.size(); j++) {
	    	if (connect(sockfd, (struct sockaddr *)&rep_addrs[j], 
				sizeof(rep_addrs[j])) == 0) {
	    		std::stringstream reqStr;
	    		reqStr << "PROMOTE " << ip_to_string(rep_addrs[j]) << "\r\n";

	    		multicastNodes(reqStr.str());
	    		primary_addr = rep_addrs[j];
	    		conn = new	masterConnection(sockfd, 0);
	    		running = true;

	    		std::cout <<"node: " << nodeID <<" new primary " << ip_to_string(rep_addrs[j]) << std::endl;
	    		
	    		break;
	    	}
	    }
	}

	bool multicastNodes(string command) {
		int sockfd = socket(PF_INET, SOCK_STREAM, 0);
		for (int j = 0; j < rep_addrs.size(); j++) {
		
			if (connect(sockfd, (struct sockaddr *)&rep_addrs[j], 
				sizeof(rep_addrs[j])) == 0) {
				masterConnection conn(sockfd, 0);
				std::string response;
				contactServer(conn, command, response);
			}


		}

		close(sockfd);

		return true;

	}

	bool contactServer(masterConnection& connection,const std::string& request, std:: string& response) {
		connection.do_write(request.c_str(), request.length());

		int rlen = 1000;
		char buf[rlen];
		memset(buf, 0, rlen);

		connection.do_read(buf, rlen);
		
		response = string(buf);

		std::string QUIT = "QUIT\r\n";
		connection.do_write(QUIT.c_str(), QUIT.length());

		return true;

	}

};




#endif /* MASTER_MASTER_H_ */
