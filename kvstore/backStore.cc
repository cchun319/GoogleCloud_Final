/*
 * backStore.cc
 *
 *  Created on: 2018年4月13日
 *      Author: BearBurg
 */

#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>

#include "backStore.h"
#include "filetool.h"

BackStore::BackStore(std::vector<struct sockaddr_in> list, int myID) {

	this->myID = myID;

	//establish connection to replicates
	establishConn(list);

}

bool BackStore::establishConn(std::vector<struct sockaddr_in> list) {
	for (int i = 0; i < list.size(); i++) {
		if (i == myID) {
			continue;
		}

		int sockfd = socket(PF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) {
			std::cerr << "Cannot open socket" << std::endl;
			return false;
		}

		if (connect(sockfd, (struct sockaddr*) &list[i],
				sizeof(list[i])) != -1) {
			Connection* c = new Connection(sockfd, 0);
			connections.push_back(c);

			Replicate* r = new Replicate(list[i], c);
			nodes.push_back(r);

		} else {
			Replicate* r = new Replicate(list[i], NULL);
			r->state = 3;
			nodes.push_back(r);
		}
		

	}

	return true;
}

bool BackStore::push(std::string row, std::string col, std::string* data, long seq) {

	std::stringstream tem;
	tem << "PUT <" << row << "> <" << col << ">\r\n" << seq <<  "\r\n" << *data << ".\r\n";
	std::string command = tem.str();

	// auto it = connections.begin();
	// for (it; it != connections.end(); it ++) {
	// 	int ret;
	// 	ret = (*it)->do_write(command.c_str(), command.length());
	// }

	auto it = nodes.begin();
	for (it; it != nodes.end(); it ++) {
		Connection* conn = (*it)->connection;
		if ((*it)->state == 1) {
			bool ret = conn->do_write(command.c_str(), command.length());
			// (*it)->do_write(data->c_str(), data->length());
			// string ending = ".\r\n";
			// bool ret = (*it)->do_write(ending.c_str(), ending.length());

			if (ret == false) {
				std::cerr << "node fails" << std::endl;
				(*it)->state = 3;
			}
		} else if ((*it)->state == 2) {
			(*it)->holdBack.push_back(command);
		}
	}
	return true;
}

bool BackStore::poll(std::string row, std::string col, long seq) {
	std::stringstream tem;
	tem << "DELETE <" << row << "> <" << col << ">\r\n" << seq << "\r\n";
	std::string command = tem.str();

	auto it = nodes.begin();
	for (it; it != nodes.end(); it ++) {
		Connection* conn = (*it)->connection;
		if ((*it)->state == 1 && conn != NULL) {
			bool ret = conn->do_write(command.c_str(), command.length());
			
			if (ret == false) {
				std::cerr << "node fails" << std::endl;
				(*it)->state = 3;
			}
		} else if ((*it)->state == 2) {
			(*it)->holdBack.push_back(command);
		}
	}

	return true;
}

bool BackStore::reconnect(std::string address) {
	std::cout << address << std::endl;
	auto it = nodes.begin();
	for (it; it != nodes.end(); it ++) {
		std::cout << ip_to_string((*it)->addr) << std::endl;
		if (ip_to_string((*it)->addr) == address) {

			int sockfd = socket(PF_INET, SOCK_STREAM, 0);
			if (sockfd < 0) {
				std::cerr << "Cannot open socket" << std::endl;
				return false;
			}

			while (connect(sockfd, (struct sockaddr*) &(*it)->addr,
					sizeof((*it)->addr)) == -1) {
				sleep(1);
				std::cerr << "Try to connect to " << address << " again" << std::endl;
			}

			std::cout << "Connection established" << std::endl;
			Connection* c = new Connection(sockfd, 0);
			delete (*it)->connection;
			(*it)->connection = c;

			std::cout << "Sending holdBack" << std::endl;
			while(!(*it)->holdBack.empty()) {
				string str = (*it)->holdBack.front();
				c->do_write(str.c_str(), str.length());

				(*it)->holdBack.pop_front();
			}

			for (auto str : (*it)->holdBack) {
				c->do_write(str.c_str(), str.length());
			}

			(*it)->state = 1;


			return true;

		}
	}

	return false;
}

bool BackStore::recover(std::string address) {
	auto it = nodes.begin();
	for (it; it != nodes.end(); it ++) {
		if (ip_to_string((*it)->addr) == address) {
			std::cerr << "Set " << address << " recovering" << std::endl;
			(*it)->state = 2;
			return true;
		
		}
	}

	return false;

}