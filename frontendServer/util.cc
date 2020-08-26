/*
 * util.cc
 *
 *  Created on: Apr 23, 2018
 *      Author: cis505
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <time.h>
#include <sys/file.h>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include "request.h"
#include "response.h"
#include "util.h"

using namespace std;


/*
 * replace target substring in the original string with replacement
 */
string replace(string origin, string target, string replacement) {
	size_t index = 0;
	while (true) {
		// find the position of the substring
		index = origin.find(target, index);
		if (index == origin.npos) {
			// cannot find the target string
			break;
		}
		// replace the string
		origin.replace(index, target.length(), replacement);
		index += replacement.length(); // skip the new string
	}

	return origin;
}

/*
 * write response to client
 */
bool do_write(int socket_fd, char *buffer, int len) {
	int index = 0; // the starting index
	while (index < len) {
		int n = write(socket_fd, &buffer[index], len - index);
		if (n < 0) {
			return false;
		}
		index += n;
	}
	return true;
}

/*
 * get the html file and replace the target string in the file
 * with the replacement string
 */
string replace_get_html(string filename, unordered_map<string, string> replacement) {
	// open the file
	ifstream web(filename);

	if (!web) {
		fprintf(stderr, "cannot open given file!!!\n");
		exit(1);
	}

	string line;
	string content;

	while (getline(web, line)) {
		//replace line
		for (auto iter : replacement) {
			//replace every element in the map in line
			line = replace(line, iter.first, iter.second);
		}
		// write line to content
		content += line;
	}

	web.close();

	return content;
}

/*
 * parse the html file into a string
 */
string getHtml(string filename) {
	// open the file
	ifstream web(filename);

	if (!web) {
		fprintf(stderr, "cannot open given file!!!\n");
		exit(1);
	}


	string line;
	string content;


	while (getline(web, line)) {
		// write line to content
		content += line;
	}

	web.close();
	return content;
}


/*
 * get current time
 */
string get_time() {
	//get current time
	time_t rawtime;
	struct tm * timeinfo;
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	return asctime(timeinfo);
}

/*
 * read file into a vector of character
 */
void read_file_into_vector(vector<char>& res, string address) {
	ifstream file;
	file.open(address.c_str(), ios::binary);

	if (!file) {
		fprintf(stderr, "cannot open given file!!!\n");
		exit(1);
	}

	char data;
	while (file.get(data)) {
		// keep reading from the file
		res.push_back(data);
	}
}





