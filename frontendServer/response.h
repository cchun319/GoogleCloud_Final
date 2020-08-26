/*
 * response.h
 *
 *  Created on: Apr 1, 2018
 *      Author: cis505
 */

#ifndef RESPONSE_H_
#define RESPONSE_H_

#include <string>
#include <unordered_map>
#include "request.h"

using namespace std;

extern unordered_map<string, string> sessions;


class response {
	public:
		string status;
		string version;
		unordered_map<string, string> header;

		response(int client_fd, request req);
		void reply(string address, string type, int client_fd);
		void replace_reply(string address, string type, int client_fd,
				unordered_map<string, string> replacement);
		void reply_file(string filename, string type,
				string disposition, int client_fd);
		void handle_get(string path, int client_fd,
				unordered_map<string, string> cookies);
		void handle_post(string path, int client_fd, vector<char>* message,
				unordered_map<string, string> cookies);
		void handle_signin(string path, int client_fd, vector<char>* message,
				unordered_map<string, string> cookies);
		void handle_upload(string path, int client_fd, vector<char>* message);
		void handle_send_email(string path, int client_fd, vector<char>* message);
		void handle_signup(string path, int client_fd, vector<char>* message);
		void handle_create_newfolder(string path, int client_fd, vector<char>* message);
		void handle_move_file(string path, int client_fd, bool isFile);
		void handle_back(string path, int client_fd);
		void handle_rename(string path, int client_fd, vector<char>* message);
		void handle_stop_server(string path, int client_fd);
		void handle_start_server(string path, int client_fd);
		void handle_stop_backend(string path, int client_fd);
		void handle_start_backend(string path, int client_fd);
		void handle_delete(string path, int client_fd);
		void handle_open_directory(string path, int client_fd);
};


#endif /* RESPONSE_H_ */
