/*
 * client.h
 *
 *  Created on: Apr 13, 2018
 *      Author: cis505
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <arpa/inet.h>
#include <set>
#include <string>
#include <set>
#include <vector>
#include "utils.h"
using namespace std;

// class for email extraction and update in frontend
struct email {
	bool deleted, opened;
	string header, content, hash;
	email(){}
	email(string _header, string _content, string _hash){
		header = _header;
		content = _content;
		hash = _hash;
		deleted = false;
		opened = false;
	}
};

// mail sending to smtp server
int sendto_smpt_server(string& server_address, string& sender, set<string>& rcpts, string& data);
void handle_comm(int& sock, string& request, char* response);

// mail extraction from backend
vector<email> recvfrom_backend(string& master_address, string& user, bool inbox);
email extract_email(string& node_address, string& user, string& column_key);

// mail update in backend
void update_backend(string& master_address, string& user, set<string> delete_hash, bool inbox);
string update_hash(const string& old_hash, set<string> delete_hash);

#endif /* CLIENT_H_ */
