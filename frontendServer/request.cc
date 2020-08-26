/*
 * request.cc
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
#include <arpa/inet.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <string>
#include "request.h"

using namespace std;

//Macro
#define BUFFERSIZE 4000000


/*
 * constructor for a request
 */
request::request(int client_fd) {
	this->message = new vector<char>;
	vector<char>* buffer = new vector<char>;
	char* buffer_char = (char*) malloc(BUFFERSIZE);
	memset(buffer_char,0,BUFFERSIZE);
	int start = 0; // where the line start
	int end = 0; // where the line end
	int line = 0;
	bool message_body = false;
	// keep reading request
	while (true) {
		// find \r\n
		vector<char> end_char = {'\r', '\n'};
		auto iter = search(buffer->begin() + end, buffer->end(), end_char.begin(), end_char.end());
		if (iter != buffer->end()) {
			// buffer has a statement
			end = iter - buffer->begin();
		} else if (message_body) {
			// following is message
			if (end == buffer->size()) {
				// keep reading
				int read_characters = read(client_fd, buffer_char, BUFFERSIZE);
				printf("read_characters: %d\n", read_characters);
				if (read_characters <= 0) {
					// no more content
					delete(buffer);
					free(buffer_char);
					break;
				}
				// copy character to vector
				for (int i = 0; i < read_characters; i++) {
					buffer->push_back(buffer_char[i]);
					memset(&buffer_char[i], 0, 1);
				}
			}
			start = end;
			end = buffer->end() - buffer->begin();
		} else {
			// need to keep reading from the socket
			// append to the end of the buffer
			int read_characters = read(client_fd, buffer_char, BUFFERSIZE);
			printf("read_characters: %d\n", read_characters);
			if (read_characters <= 0) {
				// no more content
				delete(buffer);
				free(buffer_char);
				break;
			}
			// copy character to vector
			for (int i = 0; i < read_characters; i++) {
				buffer->push_back(buffer_char[i]);
				//printf("buffer[%d]: %c\n", i, buffer[i]);
				memset(&buffer_char[i], 0, 1);
			}
			continue;
		}
		if (!message_body) {
			end += 2; // skip \r\n
		}
		string receive(buffer->begin() + start, buffer->begin() + end);// get the line of statement
		//end = end + 1; // skip \n
		if (!message_body) {
			start = end; // change start and skip '\n'
		}
		char* copy = (char*) malloc(4000000);
		strcpy(copy, receive.c_str());


		// first line
		if (line == 0) {
			// copy attribute of request
			char *token;
			token = strtok(copy, " \r");
			this->method = string(token);
			token = strtok(NULL, " ");
			this->path = string(token);
			token = strtok(NULL, " ");
			this->version = string(token, strlen(token) - 1); // skip the final \n
			// print debug information
			if (debugInfo) {
				printf("[%d] Method: %s\n", client_fd, this->method.c_str());
				printf("[%d] path: %s\n", client_fd, this->path.c_str());
				printf("[%d] HTTP Version: %s\n", client_fd, this->version.c_str());
			}
			line++;
			free(copy);
		} else if (strlen(copy) > 8 && string(copy).substr(0, 8) == string("Cookie: ")) {
			// cookies
			// parse the cookie

			printf("parse cookie !!!!!\n");
			char* token;
			char cookie_copy[512];
			strcpy(cookie_copy, copy + 8);
			token = strtok(cookie_copy, " ");
			while (token != NULL) {
				char copy_token[256];
				strcpy(copy_token, token);
				char *token2;
				token2 = strtok(copy_token, "=");
				char key[128];
				strcpy(key, token2);
				token2 = strtok(NULL, "=");
				char value[128];
				if (token2[strlen(token2) - 2] == ';') {
					strncpy(value, token2, strlen(token2) - 1); // skip the ;
				} else {
					strncpy(value, token2, strlen(token2) - 2); // skip \r\n
				}

				this->cookies[string(key)] = string(value);
				token = strtok(NULL, " ");
				printf("key: %s\n", key);
				printf("value: %s\n", value);
			}
			free(copy);
		} else if (strcasecmp(copy, "\r\n") == 0) {
			// skip the blank line
			// mark the message body follows
			if (strcasecmp(this->method.c_str(), "get") == 0) {
				delete(buffer);
				free(buffer_char);
				free(copy);
				break;
			}
			message_body = true;
			free(copy);
		} else if (message_body) {
			int pos = start;
			while (pos < buffer->size()) {
				this->message->push_back(buffer->at(pos));
				pos++;
			}
			end = buffer->size();
			free(copy);
		}
	}
}

