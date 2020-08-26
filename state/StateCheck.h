#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <fstream>
using namespace std;

class StateCheck {
	
public:

	string getState(string master_addr,string messasge);
	string getList(int port, char* ip, string command);
	sockaddr_in parse_addr(string address);
	void closeAll(int port, char* ip);
	vector<string> parse_id(string str);
	vector<string> parse_all_server(string s);
	string startNodeCommand(int nodeID, int repID, bool recovery);
};
