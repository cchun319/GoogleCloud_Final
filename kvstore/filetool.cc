/*
 * filetool.cc
 *
 *  Created on: 2018年4月8日
 *      Author: BearBurg
 */
#include <iostream>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <cstring>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "filetool.h"

bool file_exist(const std::string& filename) {
	std::ifstream file(filename.c_str());
	    return file.good();
}

bool read_file(const std::string& filename, std::string& contents) {
	std::ifstream infile(filename, std::fstream::in);

	if (infile.is_open()) {
		std::stringstream in;
		in << infile.rdbuf();
		contents = in.str();
		return true;
	}
	return false;
}

bool write_file(const std::string& filename, const std::string& contents) {
	std::ofstream myfile;
	myfile.open(filename, std::fstream::out);

	if (myfile.is_open()) {
		myfile << contents;
		return true;
	}
	return false;
}

bool append_to_file(const std::string& filename, const std::string& contents) {
	std::ofstream myfile;
	myfile.open(filename, std::fstream::app);

	if (myfile.is_open()) {
		myfile << contents;
		return true;
	}

	return false;
}

bool delete_file(const std::string& filename){
	if(remove(filename.c_str()) != 0 ) {
	 	return false;
	} else {
	 	return true;
	}
}

void string_to_ip(sockaddr_in& addr, std::string s) {
	addr.sin_family = AF_INET;

	size_t pos = s.find(":");
	if (pos == std::string::npos) {
		inet_pton(AF_INET, s.c_str(), &(addr.sin_addr));
	} else {
		inet_pton(AF_INET, s.substr(0, pos).c_str(), &(addr.sin_addr));
		addr.sin_port = htons(stoul(s.substr(pos + 1)));
	}
}

std::string ip_to_string(sockaddr_in addr) {
	char ip_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(addr.sin_addr), ip_str, INET_ADDRSTRLEN);
	char str[100];
	sprintf(str, "%s:%hu", ip_str, ntohs(addr.sin_port));
	std::string ret(str);
	return ret;
}

