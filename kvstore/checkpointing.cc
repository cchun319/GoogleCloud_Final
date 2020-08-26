/*
 * checkpointing.cc
 *
 *  This class is for regular checkpoint, log and data recovery

 */
#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <stdio.h>

#include "checkpointing.h"
#include "server.h"
#include "filetool.h"
#include "serverState.h"
#include "cell.h"


extern bmap table;

CheckPointing::CheckPointing() {

}

bool CheckPointing::writeToDisk(ServerState* s) {
	std::cout << "Start check pointing" << std::endl;
	std::string filepath = s->getCheckpointingPath();
	std::string temPath = s->getFilePath()+"/tem.txt";

	std::ofstream myfile;
	myfile.open(temPath);

	if (!myfile.is_open()) {
		return false;
	}

	s->getMapLock();

	std::cout << "Lock Got" << std::endl;

	myfile << "Version number: <" << s->seqID << ">\n";
	
	bmap::iterator it;

	for (it = table.begin(); it != table.end(); it++) {
		std::string row = it->first;

		smap::iterator it2 = it->second.begin();

		while (it2 != it->second.end()) {
			std::string col = it2->first;

			myfile << "PUT <" << row << "> <" << col << ">\n";
			myfile << *(it2->second->contents) << "\n.\n";

			 it2 ++;
		}

	}

	myfile.close();

	rename(temPath.c_str(),filepath.c_str());
	std::string logPath = s->getLogPath();
	write_file(logPath, "");

	s->releaseMapLock();

	std::cout << "End check pointing" << std::endl;
	return true;

}

bool CheckPointing::recoverCheckpoint(ServerState* s) {
	std::cout << "Start replaying checkpoint" << std::endl;

	std::ifstream infile(s->getCheckpointingPath());
	std::cout << s->getCheckpointingPath() << std::endl;

	if (!infile) {
		std::cerr << "open file fails" << std::endl;
		return false;
	}

	std::string versionNumLine;
	//skip the first line
	getline(infile,versionNumLine);
	std::cout << "version" << versionNumLine<<std::endl;

	std::stringstream data;

	for (std::string line; getline(infile, line);) {
		//std::cout << "line"<< line << std::endl;
		if (line != ".") {
			//std::cout << data.str();
			data << line << "\n";
		} else {
			data << line << "\n";
			//std::cout <<"data:" <<data.str();
			replyCheckpointCommand(data);
			data.str("");
		}

	}


}

bool CheckPointing::replyCheckpointCommand(const std::stringstream& command) {
	std::string str = command.str();

	//parse first line PUT <ROW> <COL>
	size_t pos = str.find("\n");
	std::string PUTLine = str.substr(0, pos);
	std::string row = PUTLine.substr(PUTLine.find("<") + 1, PUTLine.find(">") - PUTLine.find("<") - 1);

	PUTLine = PUTLine.substr(PUTLine.find(">") + 1);
	std::string col = PUTLine.substr(PUTLine.find("<") + 1, PUTLine.find(">") - PUTLine.find("<") - 1);

	std::string* data = new std::string(str.substr(pos + 1, str.find("\n.\n") - pos - 1));

	std::cout << "row: " << row <<std::endl;
	std::cout << "col: " << col <<std::endl;
	//std::cout << "data: " << *data << "["<< data->length()<<"]";

	Cell* new_cell = new Cell(data);
	table[row][col] = new_cell;

}

bool CheckPointing::recoverLog(ServerState* s){
	std::cout << "Start replaying log" << std::endl;

	std::ifstream infile(s->getLogPath());

	if (!infile) {
		std::cerr << "open file fails" << std::endl;
		return false;
	}


	std::stringstream data;

	for (std::string line; getline(infile, line);) {
		//std::cout << "line"<< line << std::endl;
		if (line != ".") {
			//std::cout << data.str();
			data << line << "\n";
		} else {
			data << line << "\n";
			//std::cout <<"data:" <<data.str();
			replayLogCommand(data);
			data.str("");
		}

	}

}

bool CheckPointing::replayLogCommand(const std::stringstream& command) {
	bool isDelete = false;
	std::string str = command.str();

	if (str.substr(0, str.find(" ")) == "DELETE") {
		isDelete = true;
	}

	//parse first line PUT <ROW> <COL>
	size_t pos = str.find("\n");
	std::string PUTLine = str.substr(0, pos);
	std::string row = PUTLine.substr(PUTLine.find("<") + 1, PUTLine.find(">") - PUTLine.find("<") - 1);

	PUTLine = PUTLine.substr(PUTLine.find(">") + 1);
	std::string col = PUTLine.substr(PUTLine.find("<") + 1, PUTLine.find(">") - PUTLine.find("<") - 1);

	std::cout << "row: " << row <<std::endl;
	std::cout << "col: " << col <<std::endl;

	//parse seq number
	str = str.substr(pos + 1);
	pos = str.find("\n");
	std::string seqLine = str.substr(0, pos);

	if (isDelete) {
		if (table.count(row) != 0 && table.at(row).count(col) != 0) {
			table.at(row).erase(row);
		}		
	} else {
		//parse data 
		str = str.substr(pos + 1);
		std::string* data = new std::string(str.substr(0, str.find("\n.\n")));
		Cell* new_cell = new Cell(data);
		table[row][col] = new_cell;
	}
		
	
}

long CheckPointing::lastLogSeq(ServerState* s) {
	if (!file_exist(s->getLogPath())) {
		return 0;
	}

	std::string contents;
	read_file(s->getLogPath(), contents);

	size_t pos = contents.rfind(">\n");

	if (pos != std::string::npos) {
		size_t pos2 = contents.rfind(".\n", pos);
		if (pos2 != std::string::npos) pos = contents.find(">\n", pos2);
		else pos = contents.find(">\n");

		std::string seqStr = contents.substr(pos + 2);
		
		seqStr = seqStr.substr(0, seqStr.find("\n"));
		std::cout << seqStr << std::endl;

		return stol(seqStr);
	}

	return 0;
	

}

long CheckPointing::findVersionNum(ServerState* s) {
	if (!file_exist(s->getCheckpointingPath())) {
		return 0;
	}

	std::ifstream infile(s->getCheckpointingPath());
	std::cout << s->getCheckpointingPath() << std::endl;

	if (!infile) {
		std::cerr << "open file fails" << std::endl;
		return 0;
	}

	std::string versionNumLine;
	//skip the first line
	getline(infile,versionNumLine);
	
	size_t pos = versionNumLine.find("<") + 1;
	std::string numStr = versionNumLine.substr(pos , versionNumLine.find(">") - pos);

	return stol(numStr);
}
