/*
 * Worker.cc
 *
 * This class is the worker class for the server interface, supports command:
 * 1.For clients:
 *   * WHERE <ROW>\r\n -- Server <ip:port>\r\n     return the node ID storing the row
 *	 * PRIMARY [nodeID]\r\n -- +OK [ip:port]\r\n   return the server address of the node
 * 2. For admin:
 * 	 * LIST\r\n 								   return a list of row stored
 *   * STATES\r\n 								   return a list of servers' states
 */
#include "../master/masterWorker.h"

#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <cstdio>
#include <vector>
#include <stdio.h>
#include <map>
#include <algorithm>

#include "../filetool.h"
#include "../master/masterConnection.h"
#include "master.h"

using namespace std;

extern pthread_mutex_t node_index_mutex;
extern std::map<std::string, int> node_index;
extern std::vector<node> nodes_list;

bool masterWorker::closed = false;
bool masterWorker::v_flag;

/*
 * Constructor
 */
masterWorker::masterWorker(masterConnection* t, bool flag) {
	v_flag = flag;
	this->task = t;
	this->run_flag = true;
	this->rlen = 1000;

	this->waitingareabuf = new string("");
	this->mbox_filename = new string("");

}

/*
 * Clean up resources
 */
masterWorker::~masterWorker() {

	delete this->waitingareabuf;

	delete this->mbox_filename;

}

/*
 * Close worker from thread
 * */
void masterWorker::closeWorker() {
	closed = false;
}

/*
 * Main loop
 */
void masterWorker::run() {
	//greeting
	//reply("+OK  ready localhost\r\n");

	//initialize waiting area
	char wait[rlen];
	memset(wait, 0, this->rlen);

	while (run_flag && !closed) {
		char buf[rlen];
		char comm[rlen];
		memset(buf, 0, rlen);
		memset(comm, 0, rlen);

		task->do_read(buf, rlen);

		strcat(wait, buf);

		while (extract_command(wait, comm) == 1) {
			waitingareabuf->assign(wait);

			string command(comm);

			printc(command);

			string QUIT = "QUIT";
			string WHERE = "WHERE";
			string PRIMARY = "PRIMARY";
			string LIST = "LIST";
			string STATES = "STATES";

			if (strnicmp(command.c_str(), QUIT.c_str(), QUIT.length()) == 0) {
				reply("+OK server signing off\r\n");

				run_flag = false;

			} else if (strnicmp(command.c_str(), WHERE.c_str(), WHERE.length())
					== 0) {
				parse_WHERE(command);

			} else if (strnicmp(command.c_str(), PRIMARY.c_str(), PRIMARY.length())
					== 0) {
				parse_PRIMARY(command);

			} else if (strnicmp(command.c_str(), LIST.c_str(), LIST.length())
					== 0) {
				parse_LIST();

			} else if (strnicmp(command.c_str(), STATES.c_str(), STATES.length())
					== 0) {
				parse_STATES();

			} else {
				reply("-ERR Not supported\r\n");
			}

		}

	}
}

void masterWorker::parse_LIST() {
	//reply("+OK\r\n");
	for (auto it = node_index.begin(); it != node_index.end(); it ++) {
		reply(it->first + "\r\n");
	}

	reply(".\r\n");

}

void masterWorker::parse_STATES() {	
	//reply("+OK\r\n");

	std::string headline("Node|ID|IP address|status\n");

	//reply(headline);
	
	for (int i = 0; i < nodes_list.size(); i++) {
		for (int j = 0; j < nodes_list[i].rep_addrs.size(); j++) {
			char info[1000];
			std::string state = "down";
			if (isAlive(nodes_list[i].rep_addrs[j])) {
				state = "alive";
			}
						
			sprintf(info, "%d|%d|%s|%s\n", i,j,
				ip_to_string(nodes_list[i].rep_addrs[j]).c_str(), state.c_str());

			reply(std::string(info));
		}

	}

	reply("\r\n");
	
}

bool masterWorker::isAlive(sockaddr_in addr) {
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);

	if (connect(sockfd, (struct sockaddr *)&addr, 
		sizeof(addr)) == 0) {
		masterConnection conn(sockfd, 0);
		std::string QUIT = "QUIT\r\n";
		conn.do_write(QUIT.c_str(), QUIT.length());
		close(sockfd);
		return true;
	} 

	close(sockfd);
	return false;

}

void masterWorker::parse_PRIMARY(string command) {
	size_t pos = command.find(" ") + 1;
	int nodeID = atoi(command.substr(pos, command.find("\r\n") - pos).c_str());

	if (nodeID >= 0 && nodeID < nodes_list.size()) {
		string replyStr = "+OK " + ip_to_string(nodes_list[nodeID].primary_addr) + "\r\n";
		reply(replyStr);

	} else {
		reply("-ERR\r\n");
	}
}


void masterWorker::parse_WHERE(string command) {

	size_t startPos = command.find("<") + 1;
	string row = command.substr(startPos, command.find(">") - startPos);

	pthread_mutex_lock(&node_index_mutex);
	int is_exist = node_index.count(row);
	pthread_mutex_unlock(&node_index_mutex);

	if (is_exist == 0) {

		//find the node with least rows
		int min = nodes_list[0].row_num;
		int minID = 0;

		for (int i = 0; i < nodes_list.size(); i++) {
			if (nodes_list[i].row_num < min) {
				minID = nodes_list[i].nodeID;
				min = nodes_list[i].row_num;
			}
		}

		pthread_mutex_lock(&node_index_mutex);
		node_index[row] = minID;
		pthread_mutex_unlock(&node_index_mutex);

		std::stringstream o;
		o << "Row " << row << " assigned to node " << minID << endl;
		prints(o.str());

		nodes_list[minID].row_num = nodes_list[minID].row_num + 1;
		cout << nodes_list[minID].row_num << endl;

		stringstream reply_str;
		reply_str << "SERVER <" << ip_to_string(nodes_list[minID].primary_addr) << ">\r\n";
		reply(reply_str.str());
	} else {
		//pthread_mutex_lock(&node_index_mutex);
		int nodeID = node_index[row];
		//pthread_mutex_unlock(&node_index_mutex);

		std::stringstream o;
		o << "Row " << " found in node " << nodeID;
		prints(o.str());

		stringstream reply_str;
		reply_str << "SERVER <" << ip_to_string(nodes_list[nodeID].primary_addr) << ">\r\n";
		reply(reply_str.str());

	}

}

/*
 * Extracts a command ending with "\r\n", it will change the buf and command
 * */
int masterWorker::extract_command(char* buf, char* command) {
	int len = strlen(buf);
	char* s;

	if ((s = strstr(buf, "\r\n")) != NULL) {
		memset(command, 0, strlen(command));
		strncpy(command, buf, s - buf + 2);
		//if still remaining characters in buf
		if (s - buf + 2 <= len) {
			strcpy(buf, s + 2);
		}
		return 1;
	}

	return 0;
}


/*
 * Compares first n character of two strings case insensitive
 * */
int masterWorker::strnicmp(const char* s1, const char* s2, size_t n) {
	size_t min_len = n;

	size_t len1 = strlen(s1);
	if (len1 == 0)
		return 1;
	if (len1 < n)
		min_len = len1;

	size_t len2 = strlen(s2);
	if (len2 == 0)
		return 1;
	if (len2 < n)
		min_len = len2;

	for (unsigned int i = 0; i < min_len; i++) {
		if (toupper(s1[i]) != toupper(s2[i]))
			return 1;
	}
	return 0;
}

/*
 * Replies to clients
 * */
void masterWorker::reply(string s) {
	task->do_write(s.c_str(), s.length());
	prints(s);
}

/*
 * print debug info
 */
void masterWorker::printc(string s) {
	if (v_flag) {
		cerr << "[" << this->task->getCount() << "] C: " << s;
	}

}

/*
 * print debug info
 */
void masterWorker::prints(string s) {
	if (v_flag) {
		cerr << "[" << this->task->getCount() << "] S: " << s;
	}

}

