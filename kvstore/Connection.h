/*
 * Connection.h
 *
 *  Created on: Feb 27, 2018
 *      Author: cis505
 */

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

using namespace std;

class Connection{
	int m_fd;
	long count;


public:
	~Connection();
	bool do_read(char *buf, unsigned long len);
	int do_write(const char*, int);
	bool do_read_data(char*, unsigned long);

	Connection(int fd, long count){
		this->count = count;
		this->m_fd = fd;
	};

	long getCount();

};
#endif /* CONNECTION_H_ */
