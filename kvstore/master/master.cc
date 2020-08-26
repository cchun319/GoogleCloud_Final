/*
 * master.cc
 *
 *  Created on: 2018年4月8日
 *      Author: BearBurg
 */
#include "../master/master.h"

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>

#include <map>
#include <string>
#include <vector>

#include "../filetool.h"
#include "../master/masterConnection.h"
#include "../master/masterThreadPool.h"
#include "master.h"

//./masterserver -v ../serverlist.txt

void signalHandler( int signum );
bool init(std::string&);
void* heartbeatThread(void* arg);
bool contactServer(masterConnection& connection,const std::string& request, std:: string& response);
bool isAlive(sockaddr_in addr, int sockfd);
bool multicastNodes(int i, string command);

bool v_flag = false;
bool main_flag = true;
long connection_count = 0;
int listen_fd;
masterThreadPool threadPool(100);

sockaddr_in my_addr;

pthread_mutex_t nodes_list_mutex;
//a vector holds all nodes
std::vector<node> nodes_list;
//mutex for node_index
pthread_mutex_t node_index_mutex;
//a map keeping track of each nodes' index
std::map<std::string, int> node_index;

pthread_t heartbeatThreadPid;


int main(int argc, char *argv[]) {

	int c;
	while ((c = getopt(argc, argv, "vp:")) != -1) {
		switch (c) {
		case 'v':
			v_flag = true;
			break;
		case '?':
			if (optopt == 'c')
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint(optopt))
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			return 1;

		default:
			abort();
		}

	}

	//set configuration file path
	string FILE_PATH;
	if (argc <= optind) {
		cerr << "ERR missing server configuration file" << endl;
		exit(1);
	} else {
		FILE_PATH = argv[optind];
		if (!init(FILE_PATH)) {
			exit(1);
		}
		if (v_flag) {
			std::cout << "my addr: " << ip_to_string(my_addr) << std::endl;
			for (auto n : nodes_list) {
				std::cout << n.nodeID << " primary: " << ip_to_string(n.primary_addr) << " replicates: ";
				for (auto addr : n.rep_addrs) {
					std::cout << ip_to_string(addr) << std::endl;
				}
			}
		}
	}

	std::signal(SIGPIPE, SIG_IGN);
	//create heartbeat thread
	int ret;
	pthread_create(&heartbeatThreadPid, NULL, &heartbeatThread, &ret);

	listen_fd = socket(PF_INET, SOCK_STREAM, 0);
	bind(listen_fd, (struct sockaddr*) &my_addr, sizeof(my_addr));
	listen(listen_fd, 10);

	threadPool.CreatePool();
	threadPool.setFlag(v_flag);
	masterConnection* task;

	std::signal(SIGINT, signalHandler);

	while (main_flag) {
		//connect to the server
		struct sockaddr_in clientaddr;
		socklen_t clientaddrlen = sizeof(clientaddr);
		int fd;
		fd = accept(listen_fd, (struct sockaddr*) &clientaddr, &clientaddrlen);

		connection_count++;
		if (v_flag) {
			fprintf(stderr, "[%ld] Connection from %s\n", connection_count,
					inet_ntoa(clientaddr.sin_addr));
			fprintf(stderr, "[%ld] New Connection\n", connection_count);
		}

		//create task
		task = new masterConnection(fd, connection_count);
		//push task to threadpool
		threadPool.addTask(task);

	}

}

void signalHandler( int signum ) {
	cout << "Interrupt signal (" << signum << ") received.\n";

	threadPool.closeAll();
	main_flag= false;
	close(listen_fd);

	pthread_cancel(heartbeatThreadPid);
	exit(0);
}



bool init(std::string& serverlist_filename) {
	//set server list
	ifstream infile(serverlist_filename);

	if (!infile) {
		cerr << "open file fails" << endl;
		return false;
	}

	//set master's address
	string m_addr_str;
	getline(infile,m_addr_str);
	string_to_ip(my_addr, m_addr_str);

	int i = 0;

	//set node address list
	for (string line; getline(infile, line);) {
		size_t pos = line.find(",");
		sockaddr_in addr;

		//set the primary server
		string_to_ip(addr,line.substr(0,pos));
		//cout << "p:" <<ip_to_string(addr);
		node new_node(i);
		new_node.primary_addr = addr;
		new_node.rep_addrs.push_back(addr);

		string s = line.substr(pos + 1);
		

		while (pos != string::npos) {
			pos = s.find(",");
			sockaddr_in rep_addr;

			string_to_ip(rep_addr,s.substr(0, pos));
			new_node.rep_addrs.push_back(rep_addr);

			//cout << "r:" <<ip_to_string(rep_addr);

			s = s.substr(pos + 1);
		}

		nodes_list.push_back(new_node);

		i++;
	}



	//initialize mutex
	pthread_mutex_init(&node_index_mutex, NULL);
	pthread_mutex_init(&nodes_list_mutex, NULL);

	return true;


}

void* heartbeatThread(void* arg) {
	while (main_flag) {
		for (int i = 0; i < nodes_list.size(); i++ ) {
			nodes_list[i].heartbeat();
		}
		sleep(2);
	}

	
}

