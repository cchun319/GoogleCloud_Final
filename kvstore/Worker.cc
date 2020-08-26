/*
 * Worker.cc
 *
 * This class is the worker class for the server interface, supports command:
 * 1.For clients:
    * PUT <ROW> <COL>\r\n
       DATA
       [DATA]
       .\r\n
       -- +OK/-ERR     					input data
 	* CPUT <ROW> <COL>\r\n
       OLD
       [OLD DATA]
       .\r\n
       NEW
       [NEW DATA]
       .\r\n
       -- +OK/-ERR     				   input data, success only if old data equals [OLD DATA]
 	* GET <ROW> <COL>\r\n
       -- +OK/-ERR\r\n
       DATA
       [DATA]
       .\r\n    					   get data stored in [ROW][COL]
    * DELETE <ROW> <COL>\r\n
      -- +OK/-ERR\r\n 				   delete data stored in [ROW][COL]
 * 2. For admin:
   * LIST\r\n 					       return a list of row stored
   * CLOSEALL\r\n 					   kill the process	
 * 
 */
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
#include <ctype.h>
#include <algorithm>
#include <csignal>

#include "Worker.h"
#include "Connection.h"
#include "cell.h"
#include "server.h"
#include "filetool.h"
#include "backStore.h"
#include "checkpointing.h"


using namespace std;

extern ServerState* serverState;
//extern string FILE_PATH;
extern bmap table;

extern BackStore* backStore;

extern pthread_t checkpointingPID;
extern int listen_fd;
extern bool main_flag;

bool Worker::closed = false;
bool Worker::v_flag;

using namespace std;

/*
 * Constructor
 */
Worker::Worker(Connection* t, bool flag) {
	v_flag = flag;
	this->task = t;
	this->run_flag = true;
	this->rlen = 4000000;
}

/*
 * Clean up resources
 */
Worker::~Worker() {

}

/*
 * Close worker from thread
 * */
void Worker::closeWorker() {
	closed = false;
}

/*
 * Main loop
 */
void Worker::run() {
	//greeting
	//reply("+OK  ready localhost\r\n");

	while (run_flag && !closed) {
		char buf[rlen];
		memset(buf, 0, rlen);

		task->do_read(buf, rlen);

		this->w << buf;

		string command;

		while (extract_command(command) == 1) {

			printc(command);

			//from client or primary
			string QUIT = "QUIT";
			string PUT = "PUT";
			string CPUT = "CPUT";
			string GET = "GET";
			string DELE = "DELETE";
			string LIST = "LIST";

			//from master
			string ALIVE = "ALIVE";
			string PROMOTE = "PROMOTE";
			string CLOSEALL = "CLOSEALL";

			//from recovering node
			string RECOVERY = "RECOVERY";
			string READY = "READY";
			string LOG = "LOG";
			string CHECKPOINT = "CHECKPOINT";

			string row;
			string col;

			if (strnicmp(command.c_str(), QUIT.c_str(), QUIT.length()) == 0) {
				reply("+OK server signing off\r\n");

				run_flag = false;

			} else if (strnicmp(command.c_str(), PUT.c_str(), PUT.length())
					== 0) {
				parse_command(command, row, col);
				parse_PUT(row, col);

			} else if (strnicmp(command.c_str(), CPUT.c_str(), CPUT.length())
					== 0) {
				parse_command(command, row, col);
				parse_CPUT(row, col);
			} else if (strnicmp(command.c_str(), GET.c_str(), GET.length())
					== 0) {
				parse_command(command, row, col);
				parse_GET(row, col);
			} else if (strnicmp(command.c_str(), DELE.c_str(), DELE.length())
					== 0) {
				parse_command(command, row, col);
				parse_DELE(row, col);

			} else if(strnicmp(command.c_str(), ALIVE.c_str(), ALIVE.length())
					== 0) {
				parse_ALIVE();

			} else if(strnicmp(command.c_str(), PROMOTE.c_str(), PROMOTE.length())
					== 0) {
				parse_PROMOTE(command);

			} else if(strnicmp(command.c_str(), RECOVERY.c_str(), RECOVERY.length())
					== 0) {
				parse_RECOVERY(command);

			} else if(strnicmp(command.c_str(), READY.c_str(), READY.length())
					== 0) {
				parse_READY(command);

			} else if(strnicmp(command.c_str(), LOG.c_str(), LOG.length())
					== 0) {
				parse_LOG();

			} else if(strnicmp(command.c_str(), CHECKPOINT.c_str(), CHECKPOINT.length())
					== 0) {
				parse_CHECKPOINT();

			} else if(strnicmp(command.c_str(), CLOSEALL.c_str(), CLOSEALL.length())
					== 0) {
				parse_CLOSEALL();

			} else if(strnicmp(command.c_str(), LIST.c_str(), LIST.length())
					== 0) {
				parse_LIST();

			} else {
				reply("-ERR Not supported\r\n");
			}

		}

	}
}

void Worker::parse_LIST() {
	//reply("+OK\r\n");
	serverState->getMapLock();

	for (auto it = table.begin(); it != table.end(); it ++) {
		for (auto it2 = it->second.begin(); it2 != it->second.end(); it2 ++) {
			std::stringstream contents;
			contents << "<" << it->first << "> <" << it2->first << "> " << *(it2->second->contents) << "\n";
			reply(contents.str());
		}
		
	}
	serverState->releaseMapLock();
	reply(".\r\n");
}

void Worker::parse_CLOSEALL() {

	raise(SIGKILL);

}

void Worker::parse_LOG() {
	string contents;
	read_file(serverState->getLogPath(), contents);
	reply(contents);
	reply("LOG END\r\n");
}

void Worker::parse_CHECKPOINT() {
	string contents;
	read_file(serverState->getCheckpointingPath(), contents);
	reply(contents);
	reply("CHECKPOINT END\r\n");
}


void Worker::parse_READY(string command) {
	size_t pos = command.find(" ") + 1;
	std::string addrStr = command.substr(pos, command.find("\r\n") - pos); 
	//TODO mark server as running
	backStore->reconnect(addrStr);
	serverState->resumeCheckpoint();
}

void Worker::parse_RECOVERY(string command) {
	serverState->pauseCheckpoint();

	command = command.substr(command.find(" ") + 1);
	std::string addrStr = command.substr(0, command.find(" "));

	backStore->recover(addrStr);

	command = command.substr(command.find(" ") + 1);
	std::string seqStr = command.substr(0, command.find(" "));

	long startSeq = stol(seqStr);
	prints("node wants to recovery from seq" + startSeq);

	long versionNum = CheckPointing::findVersionNum(serverState);

	cout <<"vn: " <<versionNum << endl;
	cout << "seq: " << startSeq << endl;
	if (versionNum <= startSeq) {


		reply("+OK tansfering log\r\n");
		//TODO let backStore put new updates in a holdback queue for this server 
		//pause checkpoint for recovery
		//TODO pop the holdback queue

	} else {
		reply("-ERR tansfering checkpoint file\r\n");
		//pause checkpoint for recovery
		//TODO let backStore put new updates in a holdback queue for this server 
		//TODO pop the holdback queue
		//add this server to backStore
	}
	

}

void Worker::parse_PROMOTE(string command) {
	size_t pos = command.find(" ") + 1;
	string newPrimaryStr = command.substr(pos, command.find("\r\n") - pos);
	prints(newPrimaryStr);

	if (newPrimaryStr == ip_to_string(serverState->getMyAddr())) {
		prints("This server is the new primary");
		serverState->setPrimary();
		//TODO
		backStore = new BackStore(serverState->getServerList(), serverState->getRepId());
	} else {
		serverState->setPrimaryAddr(newPrimaryStr);
	}

	reply("+OK\r\n");
	
}

void Worker::parse_ALIVE() {
	//reply("+OK\r\n");
}

void Worker::parse_PUT(string row, string col) {

	//reply("+OK waiting for data\r\n");

	string buffer(w.str());
	size_t data_end = buffer.find("\r\n.\r\n");

	while (data_end == string::npos) {
		//buffer to read in
		//char buf[rlen];
		char* buf = (char* )malloc(sizeof(char) * rlen);
		//command buffer
		memset(buf, 0, rlen);
		task->do_read_data(buf, rlen);

		buffer.append(buf);
		free(buf);

		data_end = buffer.find("\r\n.\r\n");
	}

	string* data;
	long primarySeq;
	if (serverState->isPrimary()) {
		size_t pos = buffer.find("DATA\r\n") + 6;
		data = new string(buffer.substr(pos, data_end - pos + 2));
		//cout << "Data from client: " << *data;

		serverState->getMapLock();
		primarySeq = putToLog(data, row, col);

	} else {
		size_t pos = buffer.find("\r\n") + 2;
		data = new string(buffer.substr(pos, data_end - pos + 2));
		long seq = stol(buffer.substr(0, pos - 2));
		//cout << "Data from primary: " << *data;
		cout << "seq: " << seq << endl;

		serverState->getMapLock();
		//update sequence number locally
		serverState->updateSeq(seq);

		putToLog(seq, data, row, col);
	}

	std::cout << "data size :" << data->length() << std::endl;

	if (table.count(row) == 0 || table.count(col) == 0) {
		prints("new data\r\n");
		Cell* new_cell = new Cell(data);
		table[row][col] = new_cell;

	} else {
		Cell* cell = table[row][col];

		delete cell->contents;
		cell->contents = data;

	}
	serverState->releaseMapLock();	

	if (serverState->isPrimary()) {	
		backStore->push(row,col, data, primarySeq);
	}

	reply("+OK\r\n");

	// delete waitingareabuf;
	if (data_end + 5 == buffer.size()) {
		w.str("");
	} else {
		w.str(buffer.substr(data_end + 6));
	}

}

long Worker::putToLog(const std::string* data, const string& row,
		const string& col) {
	long seq;

	seq = serverState->increseSeq();

	string logPath = serverState->getLogPath();
	stringstream logStr;
	logStr << "PUT <" << row << "> <" << col << ">\n";
	logStr << seq << "\n";
	append_to_file(logPath, logStr.str());
	append_to_file(logPath, *data);
	append_to_file(logPath,"\n.\n");

	return seq;

}

long Worker::putToLog(long seq, const std::string* data, const string& row,
		const string& col) {

	string logPath = serverState->getLogPath();
	stringstream logStr;
	logStr << "PUT <" << row << "> <" << col << ">\n";
	logStr << seq << "\n";
	append_to_file(logPath, logStr.str());
	append_to_file(logPath, *data);
	append_to_file(logPath,".\n");

	return seq;

}

long Worker::deleteToLog(const string& row, const string& col) {
	long seq;

	seq = serverState->increseSeq();

	string logPath = serverState->getLogPath();
	stringstream logStr;
	logStr << "DELETE <" << row << "> <" << col << ">\n";
	logStr << seq << "\n";
	append_to_file(logPath, logStr.str());
	append_to_file(logPath,".\n");

	return seq;
}

long Worker::deleteToLog(long seq, const string& row, const string& col) {

	string logPath = serverState->getLogPath();
	stringstream logStr;
	logStr << "DELETE <" << row << "> <" << col << ">\n";
	logStr << seq << "\n";
	append_to_file(logPath, logStr.str());
	append_to_file(logPath,".\n");

	return seq;
}

void Worker::parse_GET(string row, string col) {

	if (table.count(row) == 0) {
		reply("-ERR NULL row\r\n");
	} else {
		if (table.at(row).count(col) == 0) {
			reply("-ERR NULL colmn\r\n");
		} else {
			reply("DATA\r\n");
			
			reply(table.at(row).at(col)->contents);
			std::cout << "data size :" << table.at(row).at(col)->contents->length() << std::endl;
			

			reply(".\r\n");
		}

	}
}

void Worker::parse_DELE(string row, string col) {

	long primarySeq;
	if (serverState->isPrimary()) {
		serverState->getMapLock();

		primarySeq = deleteToLog(row, col);
	} else {
		string buffer(w.str());
		size_t data_end = buffer.find("\r\n");

		while (data_end == string::npos) {
			//buffer to read in
			char buf[rlen];
			//command buffer
			memset(buf, 0, rlen);
			task->do_read_data(buf, rlen);

			buffer.append(buf);

			data_end = buffer.find("\r\n");
		}

		// delete waitingareabuf;
		if (data_end + 2 == buffer.size()) {
			w.str("");
		} else {
			w.str(buffer.substr(data_end + 2));
		}

		serverState->getMapLock();
		cout << buffer << endl;
		cout << buffer.substr(0, buffer.find("\r\n")) << endl;
		deleteToLog(std::stol(buffer.substr(0, buffer.find("\r\n"))), row, col);

	}

	if (table.count(row) == 0) {
		reply("-ERR NULL row\r\n");
		serverState->releaseMapLock();
	} else {
		if (table.at(row).count(col) == 0) {
			reply("-ERR NULL colmn\r\n");
			serverState->releaseMapLock();
		} else {
			delete table.at(row).at(col);
			table.at(row).erase(col);
			serverState->releaseMapLock();

			if (serverState->isPrimary()){
				backStore->poll(row, col, primarySeq);
			}
			reply("+OK deleted\r\n");
		}

	}


}

void Worker::parse_CPUT(string row, string col) {
    //sleep(10);
	string buffer(w.str());
	size_t data_end = buffer.find("\r\n.\r\n");

	while (data_end == string::npos) {
		//buffer to read in
		char buf[rlen];
		//command buffer
		memset(buf, 0, rlen);
		task->do_read_data(buf, rlen);

		buffer.append(buf);

		data_end = buffer.find("\r\n.\r\n");
	}

	size_t pos = buffer.find("OLD\r\n") + 5;
	string* oldData = new string(buffer.substr(pos, data_end - pos + 2));
	//printc(*oldData);

	buffer = buffer.substr((data_end + 3));
	data_end = buffer.find("\r\n.\r\n");

	while (data_end == string::npos) {
		//buffer to read in
		char buf[rlen];
		//command buffer
		memset(buf, 0, rlen);
		task->do_read_data(buf, rlen);

		buffer.append(buf);

		data_end = buffer.find("\r\n.\r\n");
	}

	pos = buffer.find("NEW\r\n") + 5;
	string* newData = new string(buffer.substr(pos, data_end - pos + 2));
	//printc(*newData);
	//reply("+OK waiting for NEW\r\n");

	// delete waitingareabuf
	if (data_end + 5 == buffer.size()) {
		w.str("");
	} else {
		w.str(buffer.substr(data_end + 6));
	}

	if (table.count(row) == 0 || table.at(row).count(col) == 0) {
		if (*oldData == "NULL\r\n") {
			prints("new data\r\n");
			Cell* new_cell = new Cell(newData);

			serverState->getMapLock();
			long primarySeq = putToLog(newData, row, col);
			table[row][col] = new_cell;

			serverState->releaseMapLock();

			if (serverState->isPrimary()){
				backStore->push(row, col, newData, primarySeq);
			}
			reply("+OK\r\n");


		} else {
			reply("-ERR\r\n");
		}
		
	} else {
		Cell* cell = table[row][col];

		serverState->getMapLock();
		if (*(table[row][col]->contents) == *oldData) {
			//allow to PUT
			
			long primarySeq = putToLog(newData, row, col);
			
			delete cell->contents;
			cell->contents = newData;
			

			serverState->releaseMapLock();
			if (serverState->isPrimary()){
				backStore->push(row, col, newData, primarySeq);
			}
			reply("+OK\r\n");
		} else {
			serverState->releaseMapLock();
			reply("-ERR\r\n");
		}
	
	}

}

/*
 * Extracts a command ending with "\r\n", it will change the buf and command
 * */
int Worker::extract_command(string& command) {
	string buffer = this->w.str();
	size_t pos = buffer.find("\r\n");

	if (pos != string::npos) {
		command = buffer.substr(0, pos + 2);
		this->w.str(buffer.substr(pos +2));

		return 1;
	}
	return 0;
}

/*
 * Compares first n character of two strings case insensitive
 * */
int Worker::strnicmp(const char* s1, const char* s2, size_t n) {
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

bool Worker::parse_command(string& command, string& row, string& col) {
	string s = command.substr(command.find(" ") + 1);
	row = s.substr(s.find("<") + 1, s.find(">") - s.find("<") - 1);

	s = s.substr(s.find(">") + 1);
	col = s.substr(s.find("<") + 1, s.find(">") - s.find("<") - 1);
	return true;
}

/*
 * Replies to clients
 * */
void Worker::reply(string s) {
	task->do_write(s.c_str(), s.length());
	prints(s);
}

void Worker::reply(string* s) {
	task->do_write(s->c_str(), s->length());
	prints(*s);
}

/*
 * print debug info
 */
void Worker::printc(string s) {
	if (v_flag) {
		cerr << "[" << this->task->getCount() << "] C: " << s;
	}

}

/*
 * print debug info
 */
void Worker::prints(string s) {
	if (v_flag) {
		cerr << "[" << this->task->getCount() << "] S: " << s;
	}

}

