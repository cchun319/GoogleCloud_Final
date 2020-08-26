/*
 * load_balancer.cc
 *
 *  Created on: Apr 23, 2018
 *      Author: cis505
 */

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
#include "admin.h"

using namespace std;


// global variables
pthread_mutex_t lock;
deque<Server> servers;
bool debugInfo;

// functional pointer
int parse_all_server(string filename);
void* check_server_state(void* arg);

/*
 * main function
 */
int main(int argc, char *argv[]) {
	// get command line argument
	int portNumber = 8090;
	int pFlag = 0;
	int n;

	while ((n = getopt(argc, argv, "vp:")) != -1) {
		switch (n) {
		case 'p':
			portNumber = atoi(optarg);
			break;
		case 'v':
			debugInfo = true;
			break;
		case '?':
			if (optopt == 'p')
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint(optopt))
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			exit(1);
		}
	}

	// check whether the user give a address file
	if (argv[optind] == NULL) {
		fprintf(stderr, "The address file was not given!!!\n");
		exit(1);
	}

	int num_of_servers = parse_all_server(string(argv[optind]));

	// create a thread to keep checking states of all servers
	pthread_t t;
	pthread_create(&t, NULL, check_server_state, NULL);

	// build server socket
	int listen_fd = socket(PF_INET, SOCK_STREAM, 0);

	if (listen_fd < 0) {
		fprintf(stderr, "Cannot create new socket!!!\n");
		return -1;
	}

	// Sever address
	struct sockaddr_in sever_addr;
	bzero(&sever_addr, sizeof(sever_addr));
	sever_addr.sin_family = AF_INET;
	sever_addr.sin_addr.s_addr = htons(INADDR_ANY );
	sever_addr.sin_port = htons(portNumber);

	// bind the socket
	int result_code = bind(listen_fd, (struct sockaddr *) &sever_addr,
			sizeof(sever_addr));
	if (result_code < 0) {
		fprintf(stderr, "Cannot bind socket!!!!\n");
		return -2;
	}

	// listen
	result_code = listen(listen_fd, 108);
	if (result_code < 0) {
		fprintf(stderr, "Cannot listen socket!!!!\n");
		return -3;
	}

	// main loop to accept http request
	while (true) {
		struct sockaddr_in client_addr;
		socklen_t client_addrlen = sizeof(client_addr);
		int client_fd = accept(listen_fd, (struct sockaddr *) &client_addr,
				&client_addrlen);


		if (debugInfo) {
			printf("[%d] New Connection\n", client_fd);
		}

		int count = 0;
		// find the least recently used server which is running
		pthread_mutex_lock(&lock);
		while (!servers.front().running) {
			// the server is not running
			Server tmp = servers.front();
			count++;
			servers.pop_front();
			servers.push_back(tmp);
			if (count == servers.size()) {
				break;
			}
		}
		pthread_mutex_unlock(&lock);
		if (count == servers.size()) {
			// no server is running
			if (debugInfo) {
				cout << "No server is running" << endl;
			}
			string server_response("HTTP/1.1 200 OK\r\n");
			server_response += string("Content-type: text/html\r\n");
			server_response += string("\r\n");
			server_response += getHtml("./html/503.html");
			char* res = (char *)malloc(10000);
			strcpy(res, server_response.c_str());
			do_write(client_fd, res, strlen(server_response.c_str()) + 1);
			free(res);
		} else {
			// find the server
			string frontend_address = servers.front().address + string(":") +
					to_string(servers.front().port);
			// put the used server to the back of the queue
			pthread_mutex_lock(&lock);
			Server tmp = servers.front();
			servers.pop_front();
			servers.push_back(tmp);
			pthread_mutex_unlock(&lock);

			if (debugInfo) {
				cout << "Server: " << frontend_address << " will handle the request" << endl;
			}

			// redirect to the frontend server
			string server_response("HTTP/1.1 307 Temporary Redirect\r\n");
			server_response += string("Location: http://") +
					frontend_address + string("\r\n\r\n");
			char* res = (char *)malloc(10000);
			strcpy(res, server_response.c_str());
			do_write(client_fd, res, strlen(server_response.c_str()) + 1);
			free(res);
		}
	}
	close(listen_fd);

	return 0;
}


/*
 * parse all the server in the file
 * return the number of servers
 */
int parse_all_server(string filename) {
  ifstream file(filename);
  string str;
  int count = 1;

  while (getline(file, str)) {
	  char address[64];
	  int port = 0;
	  // copy the address and port number
	  char copy[256];
	  strcpy(copy, str.c_str());
	  char *token;
	  token = strtok(copy, ":"); // get the address
	  strcpy(address, token);
	  token = strtok(NULL, ":"); // get the the port
	  port = atoi(token);

	  Server tmp(address, port);
	  servers.push_back(tmp);
	  count++;
  }

  return count;
}

/*
 * function running on one child thread to check states
 * of all the servers for every 5 seconds
 */
void* check_server_state(void* arg) {
	// create new socket
	while (true) {
		for (int i = 0; i < servers.size(); i++) {
			int sock = socket(PF_INET, SOCK_STREAM, 0);
			// build destination address according to the file and push it to the vector
			struct sockaddr_in dest;
			bzero(&dest, sizeof(dest));
			dest.sin_family = AF_INET;
			dest.sin_port = htons(servers.at(i).port);
			inet_pton(AF_INET, servers.at(i).address.c_str(), &(dest.sin_addr));

			// check connection
			int ret_val = connect(sock, (sockaddr*)&dest, sizeof(dest));
			cout << "port: " << servers.at(i).port << endl;
			cout << "address: " << servers.at(i).address << endl;
			if (ret_val == 0) {
				cout << "RUNNING" << endl;
				pthread_mutex_lock(&lock);
				servers.at(i).running = true;
				pthread_mutex_unlock(&lock);
			} else {
				cout << "DOWN" << endl;
				pthread_mutex_lock(&lock);
				servers.at(i).running = false;
				pthread_mutex_unlock(&lock);
			}
			close(sock);
		}

		// sleep for 5 seconds
		sleep(5);

	}

}


