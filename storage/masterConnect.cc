#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include "masterConnect.h"
using namespace std;

string* masterConnect::sendto_backend(string master_addr, char* message) {
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
	//sleep(3);
	do_write(sock, message, strlen(message));
	string response;
	do_read(sock, response);
	//response is SERVER <ip:port>CRLF
	int i = 0;
	int j = 0;
	for (; i < response.length(); i++) {
		if (response[i] == '<') {
			j = i;
		}
		if (response[i] == '>') {
			break;
		}
	}
	string addr = response.substr(j + 1, i - j - 1);
	string* s = new string(addr);
	write(sock,"QUIT\r\n",6);
	// close socket
	close(sock);

	return s;
}

sockaddr_in masterConnect::parse_addr(string address) {
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
	string token = address.substr(0,i);
	//char* token = strtok(address, ":");
	inet_pton(AF_INET, token.c_str(), &addr.sin_addr);

	// port
	//token = strtok(NULL, ":");
	token = address.substr(i+1);
	addr.sin_port = htons(atoi(token.c_str()));

	return addr;
}

void masterConnect::do_write(int fd, const char* buff, int len) {
	int sent = 0;
	while (sent < len) {
		int n = write(fd, &buff[sent], len - sent);
		if (n < 0) {
			cerr << "cannot write buffer to socket\r\n";
			exit(5);
		}
		sent += n;
	}
}

void masterConnect::do_read(int fd, string& buff) {
	string complete = "";
	int bytes = 0;
	while (1) {
		char buf[100];
		memset(buf, 0, 100);
		int r = read(fd, buf, 100);
		bytes += r;
		string put_reply(buf);
		memset(buf, 0, 100);
		complete.append(put_reply);
		if (strstr(complete.c_str(), "\r\n") != NULL) {
			buff = complete;
			break;
		}
	}
	cout<<"master reply:"<<buff<<endl;
}
