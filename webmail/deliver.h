/*
 * deliver.h
 *
 *  Created on: May 3, 2018
 *      Author: cis505
 */

#ifndef DELIVER_H_
#define DELIVER_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <vector>
#include <netdb.h>
#include "utils.h"
#include "client.h"
using namespace std;

// Message struct for easier delete and reset
struct Message{
	string data;
	bool deleted;
	Message(const string _data){
		data = _data;
		deleted = false;
	}
};

// read from local message queue, and try to send them to external host, then message queue will be updated
int mail_relay(const char* DIR);

// read email header and content from message queue
void read_mqueue(string& dir, vector<Message>& messages, vector<string>& headers);
// discard sent mail in message queue
void update_mqueue(string& dir, vector<Message>& messages, vector<string>& headers);

// parse email info for sending
void parse_header(string& header, vector<string>& info);

// forward to external SMTP server
void handle_forward(string& host, string& sender, string& rcpt, Message& message);
void parse_record(char* buffer, int& priority, char* address);


#endif /* DELIVER_H_ */
