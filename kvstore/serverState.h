#ifndef SERVERSTATE_H_
#define SERVERSTATE_H_

#include "filetool.h"


#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <string>

class ServerState {

public:

	bool primary;

	//my node ID
	int nodeID;
	//my replication ID
	int repID ;
	//list of this node's servers
	std::vector<struct sockaddr_in> server_list;

	//state of the server
	//0-initlizing 1-normal 2-recovering
	int status;

	//command sequence
	pthread_mutex_t seqIDMutex;
	long seqID;

	//lock for the whole map
	pthread_mutex_t mapMutex;

	bool canCheckpoint;

	std::string FILE_PATH;
	std::string LOG_PATH;
	std::string CHECKPOINTING_PATH;

	sockaddr_in my_addr;
	sockaddr_in master_addr;
	sockaddr_in primary_addr;

	ServerState();

	bool init(std::string configFilename);

	void showState();

	void updateSeq(long seq);

	long increseSeq();

	void getMapLock();

	void releaseMapLock();

	bool canCheckpointStatus() {
		return canCheckpoint;
	}

	void pauseCheckpoint() {
		canCheckpoint = false;
	}

	void resumeCheckpoint() {
		canCheckpoint = true;
	}

	void setNodeID(int id) {
		this->nodeID = id;
	}

	int getNodeID() {
		return this->nodeID;
	}

	std::string getCheckpointingPath() const {
		return CHECKPOINTING_PATH;
	}


	void setFilePath(const std::string& filePath) {
		FILE_PATH = filePath;
		LOG_PATH = FILE_PATH + "/log.txt";
		CHECKPOINTING_PATH = FILE_PATH + "/checkpoint.txt";
	}

	std::string getFilePath() const {
		return FILE_PATH;
	}

	bool isPrimary() const {
		return primary;
	}

	void setPrimary() {
		this->primary = true;
	}

	std::string getLogPath() const {
		return LOG_PATH;
	}

	const sockaddr_in& getMasterAddr() const {
		return master_addr;
	}

	void setMasterAddr(std::string addrString) {
		string_to_ip(this->master_addr,addrString);
	}

	const sockaddr_in& getMyAddr() const {
		return my_addr;
	}

	void setMyAddr(const sockaddr_in& myAddr) {
		my_addr = myAddr;
		my_addr.sin_family = AF_INET;
	}

	int getNodeId() const {
		return nodeID;
	}

	void setNodeId(int nodeId) {
		nodeID = nodeId;
	}

	const sockaddr_in& getPrimaryAddr() const {
		return primary_addr;
	}

	void setPrimaryAddr(std::string primaryAddr) {
		string_to_ip(this->primary_addr,primaryAddr);
	}

	int getRepId() const {
		return repID;
	}

	void setRepId(int repId) {
		repID = repId;
	}

	long getSeqId() const {
		return seqID;
	}

	void setSeqId(long seqId) {
		seqID = seqId;
	}

	const std::vector<struct sockaddr_in>& getServerList() const {
		return server_list;
	}

	void setServerList(std::vector<struct sockaddr_in> serverList) {
		server_list = serverList;
	}

	int getStatus() const {
		return status;
	}

	void setStatus(int status) {
		this->status = status;
	}
};

#endif