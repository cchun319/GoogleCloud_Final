/*
 * Connection.cc
 *
 *  Created on: Feb 27, 2018
 *      Author: cis505
 */
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Connection.h"

bool Connection::do_read(char *buf, unsigned long len) {
	unsigned long revd = 0;
	while (revd < len) {
		int n = read(this->m_fd, &buf[revd], len - revd);

		if (n < 0) {
//			printf("fail");
			return false;
		}

		revd += n;
		if (strstr(buf, "\r\n") != NULL) {
			buf[revd] = '\0';
			return true;
		}

	}
	return true;

}

bool Connection::do_read_data(char* buf, unsigned long len) {
	unsigned long revd = 0;
	while (revd < len) {
		int n = read(this->m_fd, &buf[revd], len - revd);

		if (n < 0) {
			return false;
		}

		revd += n;
		if (strstr(buf, "\r\n.\r\n") != NULL) {
			//buf[revd] = '\0';
			return true;
		}

	}
	return true;

}

int Connection::do_write(const char* buf, int len) {
	int sent = 0;
	while (sent < len) {
		int n = write(this->m_fd, &buf[sent], len - sent);
		if (n < 0)
			return false;
		sent += n;
	}
	return true;
}

long Connection::getCount() {
	return this->count;
}


Connection::~Connection() {
	close(m_fd);
}




