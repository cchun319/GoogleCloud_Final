#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <fstream>
using namespace std;

class masterConnect {
public:
	string master_address = "127.0.0.1:10000";
	masterConnect(){

	}
	string* sendto_backend(string master_addr, char* message);
	sockaddr_in parse_addr(string address);
	void do_write(int fd, const char* buff, int len);
	void do_read(int fd, string& buff);
	
};
