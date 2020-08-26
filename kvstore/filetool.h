/*
 * filetool.h
 *
 *  Created on: 2018年4月8日
 *      Author: BearBurg
 */

#ifndef FILETOOL_H_
#define FILETOOL_H_

#include <iostream>
#include <fstream>
#include <string>
#include <iostream>

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <cstring>


bool file_exist(const std::string&);
bool read_file(const std::string&, std::string&);
bool write_file(const std::string&, const std::string&);
bool append_to_file(const std::string&, const std::string&);
bool delete_file(const std::string&);
void string_to_ip(sockaddr_in&, std::string);
std::string ip_to_string(sockaddr_in);


#endif /* FILETOOL_H_ */
