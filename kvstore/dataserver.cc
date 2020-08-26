#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <csignal>
#include <iostream>
#include <ostream>
#include <string>

#include "server.h"
#include "ThreadPool.h"
#include "Connection.h"
#include "filetool.h"
#include "backStore.h"
#include "serverState.h"
#include "checkpointing.h"


/*
v - debug info
p - set log path
r - start from recovering
./dataserver [config file] [nodeID] [repID]
./dataserver -p ./node/1/1 -v ./serverlist.txt 1 1
*/

void signalHandler(int signum);
void* checkPointingThread(void* arg);
bool recoverProcess(int& recoverFd, int listen_fd);
bool contactServer(Connection& connection,const std::string& request, std:: string& response);
bool receiveCheckpoint(Connection& connection, std::string& data);
bool receiveLog(Connection& connection, std::string& data);

bool a_flag = false;
bool v_flag = false;
bool main_flag = true;
long int connection_count = 0;
int listen_fd;

bmap table;
ThreadPool threadPool(100);

//back store
BackStore* backStore;
ServerState* serverState;
pthread_t checkpointingPID;


int main(int argc, char *argv[]) {
	serverState = new ServerState();
	serverState->setStatus(1);

	int c;
	while ((c = getopt(argc, argv, "vp:r")) != -1) {
		switch (c) {
		case 'v':
			v_flag = true;
			break;
		case 'p':
			serverState->setFilePath(optarg);
			break;
		case 'r':
			serverState->setStatus(2);
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


	//set server list filename
	std::string configFilename;
	if (argc <= optind) {
		cerr << "ERR missing argument" << endl;
		exit(1);
	} else {
		configFilename = argv[optind];
		serverState->setNodeId(atoi(argv[optind + 1]));

		serverState->setRepId(atoi(argv[optind + 2]));
	}

	//config 
	serverState->init(configFilename);
	serverState->showState();

	
	//create socket
	listen_fd = socket(PF_INET, SOCK_STREAM, 0);

	int on = 1;
    int setRet = setsockopt( listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );

	int bindRet = bind(listen_fd, (struct sockaddr*) &(serverState->my_addr), 
		sizeof(serverState->my_addr));
	if (bindRet == -1) {
		close(listen_fd);
		std::cerr << "Cannot bind" << std::endl;
		exit(1);
	}
	listen(listen_fd, 10);

	threadPool.CreatePool();
	threadPool.setFlag(v_flag);
	Connection* task;

	//init backstore
	if (serverState->isPrimary()) {
		backStore = new BackStore(serverState->getServerList(),serverState->getRepId());
	}

	//recover if necessary
	if (serverState->getStatus() == 2) {
		if (v_flag) std::cout << "Start recover process" << std::endl; 
		serverState->primary = false;
		int recoverFd;

		if (!recoverProcess(recoverFd, listen_fd)) {
			std::cerr << "recover fails" << std::endl; 
			exit(1);
		}
		if (v_flag) std::cout << "recover finishes" << std::endl; 
		
	} else {
		if (file_exist(serverState->getLogPath())) delete_file(serverState->getLogPath());
		if (file_exist(serverState->getCheckpointingPath())) delete_file(serverState->getCheckpointingPath());
	}
	
	//create checkpointing thread
	int ret;
	pthread_create(&checkpointingPID, NULL, &checkPointingThread, &ret);


	//server ready to run
	serverState->setStatus(1);
	serverState->showState();

	std::signal(SIGINT, signalHandler);
	std::signal(SIGKILL, signalHandler);

	while (main_flag) {
		//connect to the server
		struct sockaddr_in clientaddr;
		clientaddr.sin_family = AF_INET;
		socklen_t clientaddrlen = sizeof(clientaddr);


		int fd;
		fd = accept(listen_fd, (struct sockaddr*) &clientaddr, &clientaddrlen);

		std::cout << (int)ntohs(clientaddr.sin_port) << std::endl;

		if (ip_to_string(clientaddr) == ip_to_string(serverState->getMasterAddr())) {

			task = new Connection(fd,0);
			close(fd);

		} else if (ip_to_string(clientaddr) == ip_to_string(serverState->getPrimaryAddr())) {
			if (v_flag) {
				std::cout << "Connection from primary node" << std::endl;
			}
			task = new Connection(fd,0);

		} else {
			connection_count++;

			if (v_flag) {
				fprintf(stderr, "[%ld] Connection from %s\n", connection_count,
						inet_ntoa(clientaddr.sin_addr));
				fprintf(stderr, "[%ld] New Connection\n", connection_count);
			}
			//create task
			task = new Connection(fd, connection_count);

		}
		//push task to threadpool
		threadPool.addTask(task);

	}

	return 0;
}

void signalHandler(int signum) {
	cout << "Interrupt signal (" << signum << ") received.\n";

	close(listen_fd);

	threadPool.closeAll();
	main_flag = false;

	pthread_cancel(checkpointingPID);

	delete backStore;
	delete serverState;
	exit(0);
}

void* checkPointingThread(void* arg) {
	
	while(main_flag) {
		sleep(30);

		if (serverState->canCheckpointStatus()) {
			CheckPointing::writeToDisk(serverState);
		}
		
	}

	pthread_exit(0);
}

/*
* Recover step:
	1. try to contact master server
	2. ask master about the new primary
	3. try to contact the new primary node
	4. look at local log, find the latest sequence number
	5. send recover request to primary (with the latest log number)
	6. if reply: +OK -- replay local checkpoint and log
	             otherwise -- request log and checkpoint from primary, 
	             reply checkpoint and log
	7. report "Ready" to priamry node, waiting for updates from primary's holdback
		queue

*/
bool recoverProcess(int& recoverFd, int listen_fd) {
	
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	Connection connWithMaster(sockfd,0);
	if (sockfd < 0) {
		std::cerr << "Cannot open socket" << std::endl;
		return false;
	}

	if (connect(sockfd, (struct sockaddr*)&serverState->getMasterAddr(),
			sizeof(serverState->getMasterAddr())) != -1) {

		if (v_flag) std::cout << "try to ask master about the new primary" << std::endl;
		//contact master asking for new primary address
		std::stringstream requestStr;
		requestStr << "PRIMARY " << serverState->getNodeId() << "\r\n";
		
		std::string response;
		contactServer(connWithMaster, requestStr.str(), response);

		std::string newPrimaryAddr;
		if (response.substr(0, response.find(" ")) == "+OK") {
			newPrimaryAddr = response.substr(response.find(" ") + 1, response.find("\r\n") - response.find(" ") - 1);
			std::cout << newPrimaryAddr << std::endl;
			serverState->setPrimaryAddr(newPrimaryAddr);
			std::cout << ip_to_string(serverState->getPrimaryAddr()) << std::endl;


		} else {
			std::cerr << "Cannot find new primary node!" << std::endl;
			return false;
		}
		
		std::string QUIT ="QUIT\r\n";
		contactServer(connWithMaster, QUIT, response);
	} else {
		std::cerr << "No connection with master" << std::endl;
		return false;
	}

	if (v_flag) std::cout << "Try to contact primary " << std::endl;
	//find the largest seq in log
	
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	Connection connWithPrimary(sockfd,0);

	std::stringstream waitingbuffer;
	if (sockfd < 0) {
		std::cerr << "Cannot open socket" << std::endl;
		return false;
	}

	long seq = CheckPointing::lastLogSeq(serverState);
	//long seq = 0;

	if (connect(sockfd, (struct sockaddr*)&serverState->getPrimaryAddr(),sizeof(serverState->getPrimaryAddr())) != -1)
    {

		std::stringstream requestStr;
		requestStr << "RECOVERY " << seq << " " << ip_to_string(serverState->my_addr) << "\r\n";
		std::string response;

		contactServer(connWithPrimary, requestStr.str(), response);

		//recover checkpoint
		if (response.substr(0, response.find(" ")) == "+OK") {
			std::cout << "response" << response;
			CheckPointing::recoverCheckpoint(serverState);	
			CheckPointing::recoverLog(serverState);
		} else {
			std::cerr << "Try to receive checkpoint from primary" << std::endl;
			std::string request = "CHECKPOINT\r\n";
			connWithPrimary.do_write(request.c_str(), request.length());

			std::string contents;

			receiveCheckpoint(connWithPrimary, contents);
			write_file(serverState->getCheckpointingPath(), contents);
			CheckPointing::recoverCheckpoint(serverState);
		}

		std::cout << "checkpoint replay finish" << endl;

		//recover log
		std::string request = "LOG\r\n";
		connWithPrimary.do_write(request.c_str(), request.length());

		std::string logContents;
		receiveLog(connWithPrimary, logContents);
		write_file(serverState->getLogPath(), logContents);
		CheckPointing::recoverLog(serverState);

		std::cout << "LOG replay finish" << endl;

		std::string READY = "READY " +  ip_to_string(serverState->my_addr) + "\r\n";
		std::string QUIT ="QUIT\r\n";

		sockaddr_in newPrimary;
		socklen_t addrlen = sizeof(newPrimary);

		connWithPrimary.do_write(READY.c_str(), READY.length());

		//recoverFd = accept(listen_fd, (struct sockaddr*) &newPrimary, &addrlen);

		connWithPrimary.do_write(QUIT.c_str(), QUIT.length());
		
	} else {
		std::cerr << "No connection with primary node" << std::endl;
		return false;
	}

	

	return true;

}


bool contactServer(Connection& connection,const std::string& request, std:: string& response) {
	connection.do_write(request.c_str(), request.length());

	unsigned long rlen = 1000;
	char buf[rlen];
	memset(buf, 0, rlen);

	connection.do_read(buf, rlen);
	
	response = string(buf);

	return true;

}

bool receiveCheckpoint(Connection& connection, std::string& data) {
	std::stringstream buffer;
	unsigned long rlen = 1000;
	char buf[rlen];

	do {
		memset(buf, 0, rlen);
		connection.do_read(buf, rlen);

		buffer << buf;

	} while (strstr(buf, "CHECKPOINT END\r\n") == NULL);

	std::string tem = buffer.str();
	data = tem.substr(0, tem.find("CHECKPOINT END"));

}

bool receiveLog(Connection& connection, std::string& data) {
	std::stringstream buffer;
	unsigned long rlen = 1000;
	char buf[rlen];

	do {
		memset(buf, 0, rlen);
		connection.do_read(buf, rlen);

		buffer << buf;
		std::cout << buffer.str() << std::endl;

	} while (strstr(buf, "LOG END\r\n") == NULL);

	std::string tem = buffer.str();
	data = tem.substr(0, tem.find("LOG END"));

}

