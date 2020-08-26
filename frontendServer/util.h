/*
 * util.h
 *
 *  Created on: Apr 23, 2018
 *      Author: cis505
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

extern string username;

//function
bool do_write(int socket_fd, char *buffer, int len);
string replace(string origin, string target, string replacement);
string replace_get_html(string filename, unordered_map<string, string> replacement);
string getHtml(string filename);
void read_file_into_vector(vector<char>& res, string address);
string get_time();


#endif /* UTIL_H_ */
