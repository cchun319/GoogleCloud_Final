/*
 * frontend.cc
 *
 *  Created on: Apr 1, 2018
 *      Author: cis505
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "request.h"
#include "response.h"

using namespace std;

// Marco
#define BUFFERSIZE 20480


// function pointer
void* handle_connection(void* arg);

// global variable
bool debugInfo = false;

/*
 * main function
 */
int main(int argc, char *argv[]) {
	// get command line argument
	int portNumber = 8080;
	int pFlag = 0;
	int n;

	while ((n = getopt(argc, argv, "vp:")) != -1) {
		switch(n) {
			case 'p':
				portNumber = atoi(optarg);
				break;
			case 'v':
				debugInfo = true;
				break;
			case '?':
				if (optopt== 'p')
					  fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
					  fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					  fprintf (stderr,
							  "Unknown option character `\\x%x'.\n",
							  optopt);
				exit(1);
		}
	}

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
	sever_addr.sin_addr.s_addr = htons(INADDR_ANY);
	sever_addr.sin_port = htons(portNumber);

	int on = 1;
	int setRet = setsockopt( listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );

	// bind the socket
	int result_code = bind(listen_fd, (struct sockaddr *)&sever_addr, sizeof(sever_addr));
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
		int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addrlen);
		if (debugInfo) {
			printf("[%d] New Connection\n", client_fd);
		}
		// set the time out for the socket
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

		pthread_t thread;
		pthread_create(&thread, NULL, handle_connection, &client_fd);
	}

	return 0;
}

/*
 * worker function to handle client connection
 */
void* handle_connection(void *arg) {
	int client_fd = *(int *)arg;

	// create new request
	request req(client_fd);
	response res(client_fd, req);
	close(client_fd);
	pthread_exit(NULL);
}



