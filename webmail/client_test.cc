/*
 * client_test.cc
 *
 *  Created on: Apr 14, 2018
 *      Author: cis505
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include <set>
#include "utils.h"
#include "client.h"
using namespace std;

// global
const int BUFF_SIZE = 5000;
bool DATA;

//This is a test program enable testing on 3 SMTP client functions and SMTP server, used from terminal
int main(int argc, char *argv[])
{
	if (argc != 3){
		cerr <<"Syntax: "<< argv[0] << " <smtp server> <master server>\r\n";
		exit(1);
	}
    string master_address = argv[2];

	// create a new socket(TCP)
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0){
		cerr << "cannot open socket\r\n";
		exit(2);
	}
	// set port for reuse
	const int REUSE = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &REUSE, sizeof(REUSE));

	// parse command and connect to server
	struct sockaddr_in dest;
	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_INET;

	string server_address(argv[1]);
	size_t found = server_address.find(':');
	string ip = server_address.substr(0, found);
	string port = server_address.substr(found+1);
	inet_pton(AF_INET, ip.c_str() , &dest.sin_addr);
	dest.sin_port = htons(atoi(port.c_str()));

	connect(sock, (struct sockaddr*)&dest, sizeof(dest));
	do_write(sock, "HELO tester\r\n", 13);

	// source address
	struct sockaddr_in src;
	socklen_t src_size = sizeof(src);


	while (true){
		// use select for I/O waiting, reference: https://www.gnu.org/software/libc/manual/html_node/Waiting-for-I_002fO.html
		fd_set read_set;

		FD_ZERO(&read_set);
		FD_SET(STDIN_FILENO, &read_set); // keyboard
		FD_SET(sock, &read_set); // socket

		// block until I/O is ready
		int readable = select(sock+1, &read_set, NULL, NULL, NULL);

		char buff[BUFF_SIZE];
		int read_len;

		if (FD_ISSET(STDIN_FILENO, &read_set)) {
			// keyboard input
			read_len = read(STDIN_FILENO, buff, BUFF_SIZE);
			buff[read_len-1] = 0; // remove \n at the end

			string line(buff);
			size_t found = line.find_first_of(" ");
			string command = line.substr(0, found);
			string content = line.substr(found+1);

			// here you can either follow normal SMTP protocol, or use test123 for dummy test
			if (command.compare("/quit")==0) {
				do_write(sock, "QUIT\r\n", 6);

				memset(buff, 0, BUFF_SIZE);
				read_len = read(sock, buff, BUFF_SIZE);
				cout << buff << endl;

				break;
			} else if (command.compare("/from")==0){
				string message = "MAIL FROM:<" + content + ">\r\n";
				do_write(sock, message.c_str(), message.length());
			} else if (command.compare("/to")==0){
				string message = "RCPT TO:<" + content + ">\r\n";
				do_write(sock, message.c_str(), message.length());
			} else if (command.compare("/test1")==0){
			    // test for sending function
			    string sender = "ling@localhost";
			    set<string> rcpts; rcpts.insert("linhphan@localhost"); rcpts.insert("wudao@localhost");
			    string data = "From: Wudao Ling <ling@localhost>\r\nTo: linhphan <linhphan@localhost>\r\nDate: Fri, 21 Oct 2016 18:29:11 -0400\r\nSubject: Testing my new email account\r\nLinhphan,\r\nI just wanted to see whether my new email account works.\r\n            - ling\r\n";
			    sendto_smpt_server(server_address, sender, rcpts, data);
			} else if (command.compare("/test2")==0){
				// test for extract function
				string user = "linhphan";
				vector<email> emails= recvfrom_backend(master_address, user, true);
				if (emails.empty()) cout << user << "'s inbox is empty\n";
				for (auto& mail: emails)
					cout << mail.hash << '\n' <<mail.header << endl;
				// this should be empty
				user = "ling";
				emails= recvfrom_backend(master_address, user, true);
				if (emails.empty()) cout << user << "'s inbox is empty\n";
				emails= recvfrom_backend(master_address, user, false);
				if (emails.empty()) cout << user << "'s outbox is empty\n";
				for (auto& mail: emails)
					cout << mail.hash << '\n' <<mail.header << endl;

			} else if (command.compare("/test3")==0){
				// test for update function
				string user = "linhphan";
				set<string> delete_hash; delete_hash.insert("23bab16213b5bb63b");
				update_backend(master_address, user, delete_hash, true);

				user = "ling";
				update_backend(master_address, user, delete_hash, false);

			} else if (command.compare("/data")==0){ // init data
				DATA = true;
				do_write(sock, "DATA\r\n", 6);
			} else if (DATA){
				if (line.compare("/end")==0) {
					DATA = false;
					do_write(sock, ".\r\n", 3);

					memset(buff, 0, BUFF_SIZE);
					read_len = read(sock, buff, BUFF_SIZE);
					cout << buff << endl;

					do_write(sock, "RSET\r\n", 6);
				} else {
					line = line + "\r\n";
				    do_write(sock, line.c_str(), line.length());
				}
			}

		} else {
			// response from server
			read_len = read(sock, buff, BUFF_SIZE);
			cout << buff << endl;
		}
		// clear buffer by setting zeros
		memset(buff, 0, BUFF_SIZE);
	}

	// close socket
	close(sock);
	return 0;
}



