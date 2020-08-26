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
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

using namespace std;

class masterConnection{
public:
	int m_fd;
	long count;

	~masterConnection();

	int do_write(const char*, int);
	bool do_read(char*, int);
	bool do_read_data(char*, int);

	masterConnection(int fd, long count){
		this->count = count;
		this->m_fd = fd;
	};

	long getCount();

};
#endif /* CONNECTION_H_ */
