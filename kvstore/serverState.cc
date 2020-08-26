#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <string>

#include "filetool.h"
#include "serverState.h"

ServerState::ServerState() {
	this->primary = false;
	this->nodeID = 0;
	this->repID = 0;
	this->status = 0;
	this->seqID = 0;
	this->canCheckpoint = true;
	pthread_mutex_init(&seqIDMutex, NULL);
	pthread_mutex_init(&mapMutex, NULL);
	FILE_PATH = ".";
	LOG_PATH = FILE_PATH + "/log.txt";
	CHECKPOINTING_PATH = FILE_PATH + "/checkpoint.txt";

}

void ServerState::getMapLock() {
	pthread_mutex_lock(&mapMutex);
}

void ServerState::releaseMapLock() {
	pthread_mutex_unlock(&mapMutex);
}

bool ServerState::init(std::string configFilename) {
	//set server list

	std::ifstream infile(configFilename);

	if (!infile) {
		std::cerr << "open file fails" << std::endl;
		return false;
	}

	//set master's address
	std::string m_addr_str;
	getline(infile, m_addr_str);

	string_to_ip(master_addr,m_addr_str);

	int i = 0;

	for (std::string line; getline(infile, line);) {
		if (i != nodeID) {
			i++;
			continue;
		}

		size_t pos;

		while (pos != std::string::npos) {
			pos = line.find(",");
			sockaddr_in addr;
			string_to_ip(addr, line.substr(0, pos));

			server_list.push_back(addr);

			line = line.substr(pos + 1);

		}

	}

	my_addr = server_list[repID];
	primary_addr = server_list[0];

	// if (repID == 0) {
	// 	primary = true;
	// }

	return true;
}

void ServerState::showState() {
	std::cout << "my ip: " << ip_to_string(my_addr) << std::endl;
	std::cout << "primary ip: " << ip_to_string(primary_addr) << std::endl;
	std::cout << "master ip: " << ip_to_string(master_addr) << std::endl;

	std::cout << "my node id: " << nodeID << std::endl;
	std::cout << "my rep id: " << repID << std::endl;
	
	std::cout << "state: " << status << std::endl;
	std::cout << "Primary: " << primary << std::endl;
	std::cout << "seqID: " << seqID <<std::endl;

	std::cout << FILE_PATH << std::endl;
}


void ServerState::updateSeq(long seq) {
	pthread_mutex_lock(&seqIDMutex);
	seqID = std::max(seqID, seq);
	pthread_mutex_unlock(&seqIDMutex);
}

long ServerState::increseSeq() {
	long ret;

	pthread_mutex_lock(&seqIDMutex);
	seqID = seqID + 1;
	ret = seqID;
	pthread_mutex_unlock(&seqIDMutex);

	return ret;

}
