#ifndef __utils_h__
#define __utils_h__

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include <iostream>
#include <openssl/md5.h>
using namespace std;

// communication with primary node in backend
string sendto_backend(string& master_addr, const char* message);
void do_write(int fd, const char* buff, int len);
string do_read(int fd);

// communication with master, ask where is primary
string ask_master(string& master_address, string &user);

// hashing function for email
void computeDigest(char *data, int dataLengthBytes, unsigned char *digestBuffer);
string hash_MD5(string& data);

void clear_buffer(char *buff, char *end);

#endif /* defined(__utils_h__) */
