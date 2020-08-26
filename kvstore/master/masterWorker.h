/*
 * Worker.h
 *
 *  Created on: Feb 27, 2018
 *      Author: cis505
 */

#ifndef WORKER_H_
#define WORKER_H_

#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <map>
#include <vector>
#include <fstream>
#include <iterator>

#include "../master/masterConnection.h"
#include "master.h"

/*
 * worker deals with all the commands, created by each working thread
 * */
class masterWorker {
	static bool closed;
	static bool v_flag;


public:
	masterConnection* task;



	masterWorker(masterConnection*, bool);
	~masterWorker();



	void run();
	void closeWorker();

private:
	//run flag
	bool run_flag;
	//rev_mode on when mail transaction starts

	//buffer size
	unsigned short rlen;
	std::string* waitingareabuf;
	std::string* mbox_filename;

	void parse_WHERE(std::string command);
	void parse_PRIMARY(std::string command);
	void parse_LIST();
	void parse_STATES();

	bool isAlive(sockaddr_in addr);

	static int strcmpi(const char*, const char*);
	int extract_command(char* buf, char* command);
	static int strnicmp(const char*, const char*, size_t);

	bool file_exist(const std::string*);

	void printc(std::string);
	void prints(std::string);
	void reply(std::string);


};


#endif /* WORKER_H_ */
