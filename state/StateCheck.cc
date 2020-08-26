#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "StateCheck.h"
#include "../storage/masterConnect.h"

using namespace std;

string StateCheck::getState(string master_addr, string message) {
	//string message = "states\r\n";
	sockaddr_in master = parse_addr(master_addr);
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		cerr << "cannot open socket\r\n";
		exit(4);
	}
	// set port for reuse
	const int REUSE = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &REUSE, sizeof(REUSE));
	// connect to master
	connect(sock, (struct sockaddr*) &master, sizeof(master));
	write(sock, message.c_str(), message.length());
	char buff[1000];
	string result = "";
	while (1) {
		memset(buff, 0, 1000);
		read(sock, buff, 1000);
		string buffer(buff);
		result.append(buffer);
		if (strstr(result.c_str(), "\r\n") != NULL) {
			memset(buff, 0, 1000);
			break;
		}
	}
//	cout << "result is:" << result << endl;
	return result;
}

string StateCheck::getList(int port, char* ip, string command) {
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		fprintf(stderr, "Cannot open socket (%s)\n", strerror(errno));
		exit(1);
	}
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &servaddr.sin_addr);
	servaddr.sin_port = htons(port);
	while (connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1) {
		close(sockfd);
		sockfd = socket(PF_INET, SOCK_STREAM, 0);
	}
	write(sockfd, command.c_str(), command.length());
	char buffer[1000];
	string complete = "";
	while (1) {
		memset(buffer, 0, 1000);
		read(sockfd, buffer, 1000);
		string str(buffer);
		complete.append(str);
		if (strstr(complete.c_str(), "\r\n") != NULL) {
			memset(buffer, 0, 1000);
			break;
		}
	}
	close(sockfd);
	return complete;
}

void StateCheck::closeAll(int port, char* ip) {
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		fprintf(stderr, "Cannot open socket (%s)\n", strerror(errno));
		exit(1);
	}
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &servaddr.sin_addr);
	servaddr.sin_port = htons(port);
	while (connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1) {
		close(sockfd);
		sockfd = socket(PF_INET, SOCK_STREAM, 0);
	}
	string command = "closeall\r\n";
	write(sockfd, command.c_str(), command.length());
	close(sockfd);
}

sockaddr_in StateCheck::parse_addr(string address) {
	// init a sock address
	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;

	// IP
	int i = 0;
	for (; i < address.length(); i++) {
		if (address[i] == ':')
			break;
	}
	string token = address.substr(0, i);
	inet_pton(AF_INET, token.c_str(), &addr.sin_addr);

	// port
	token = address.substr(i + 1);
	addr.sin_port = htons(atoi(token.c_str()));

	return addr;
}

vector<string> StateCheck::parse_id(string str) {
	vector<string> res;
	int current = 0;
	int previous = 0;
	for (; current < str.length(); current++) {
		if (str[current] == '|') {
			string temp = str.substr(previous, current - previous);
			res.push_back(temp);
			previous = current + 1;
		}
	}
	string temp1 = str.substr(previous);
	res.push_back(temp1);
	return res;
}

vector<string> StateCheck::parse_all_server(string s) {
	string str = s.substr(0,s.length() - 2);
	vector<string> res;
	int current = 0;
	int previous = 0;
	for (; current < str.length(); current++) {
		if (str[current] == '\n') {
			string temp = str.substr(previous, current - previous);
			res.push_back(temp);
			previous = current + 1;
		}
	}
	return res;
}

string StateCheck::startNodeCommand(int nodeID, int repID, bool recovery) {
	char command[1024];
	memset(command, 0, 1024);
	if (recovery) {
		sprintf(command, "../kvstore/dataserver -p ../kvstore/node/%d/%d -r ../kvstore/serverlist.txt %d %d",
					nodeID, repID, nodeID, repID);
	} else {
		sprintf(command, "../kvstore/dataserver -p ../kvstore/node/%d/%d ../kvstore/serverlist.txt %d %d",
							nodeID, repID, nodeID, repID);
	}


	return string(command);

}
