/*
 * Connection.cc
 *
 *  Created on: Feb 27, 2018
 *      Author: cis505
 */
#include "../master/masterConnection.h"

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>


bool masterConnection::do_read(char *buf, int len) {
	int revd = 0;
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

bool masterConnection::do_read_data(char* buf, int len) {
	int revd = 0;
	while (revd < len) {
		int n = read(this->m_fd, &buf[revd], len - revd);

		if (n < 0) {
			return false;
		}

		revd += n;
		if (strstr(buf, "\r\n.\r\n") != NULL) {
			buf[revd] = '\0';
			return true;
		}

	}
	return true;

}

int masterConnection::do_write(const char* buf, int len) {
	int sent = 0;
	
	while (sent < len) {
		int n = write(this->m_fd, &buf[sent], len - sent);
		if (n < 0)
			return false;
		sent += n;
	}
	return true;
}

long masterConnection::getCount() {
	return this->count;
}

masterConnection::~masterConnection() {
	close(m_fd);
}




