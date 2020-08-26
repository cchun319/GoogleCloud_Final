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

#include "Connection.h"
#include "filetool.h"
#include "serverState.h"


/*
 * worker deals with all the commands, created by each working thread
 * */
class Worker {
	static bool closed;
	static bool v_flag;


public:
	Connection* task;

	Worker(Connection*, bool);
	~Worker();

	void run();
	void closeWorker();

private:
	//run flag
	bool run_flag;
	//rev_mode on when mail transaction starts

	//buffer size
	unsigned long rlen;
	stringstream w;

	void parse_PUT(std::string, std::string);
	void parse_CPUT(std::string, std::string);
	void parse_GET(std::string, std::string);
	void parse_DELE(std::string, std::string);
	void parse_ALIVE();
	void parse_RECOVERY(std::string command);
	void parse_PROMOTE(std::string command);
	void parse_READY(std::string command);
	void parse_LOG();
	void parse_CHECKPOINT();
	void parse_CLOSEALL();
	void parse_LIST();

	static int strcmpi(const char*, const char*);
	int extract_command(string&);
	static int strnicmp(const char*, const char*, size_t);
	bool parse_command(string& command, string& row, string& col);

	long putToLog(const std:: string* data, const string& row, const string& col);
	long putToLog(long seq, const std:: string* data, const string& row, const string& col);
	long deleteToLog(const string& row, const string& col);
	long deleteToLog(long seq, const string& row, const string& col);

	void printc(std::string);
	void prints(std::string);
	void reply(std::string*);
	void reply(std::string);


};


#endif /* WORKER_H_ */
