/*
 * response.cc
 *
 *  Created on: Apr 1, 2018
 *      Author: cis505
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <time.h>
#include <signal.h>
#include <sys/file.h>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <deque>
#include <sstream>
#include <unordered_map>
#include <mutex>
#include <set>
#include "request.h"
#include "response.h"
#include "util.h"
#include "admin.h"
#include "../storage/Storage.h"
#include "../webmail/client.h"
#include "../state/StateCheck.h"
using namespace std;

//Macro
#define HOME "/"
#define SIGNIN_CSS "/css/signin.css"
#define EMAIL "/email"
#define NEWEMAIL "/newemail"
#define UPLOAD "/upload"
#define FILES "/files"
#define DOWNLOAD "/download?"
#define OPENDIR "/opendir"
#define LOGOUT "/logout"
#define SIGNUP "/signup"
#define MOVEFILE "/movefile"
#define RENAME "/rename"
#define KILL "/kill"
#define MOVEFOLDER "/movefolder"
#define BACK "/back"
#define MOVEFOLDERCHOOSEFOLDER "/foldermove"
#define MOVEFILECHOOSEFOLDER "/filemove"
#define ADMIN "/admin"
#define CREATEFOLDER "/createFolder"
#define DELETE "/delete"
#define STOPSERVER "/stopserver"
#define STARTSERVER "/startserver"
#define SHOWINBOX "/showinbox"
#define SHOWSENT "/showsent"
#define SHOWTRASH "/showtrash"
#define REPLY "/reply"
#define FORWARD "/forward"
#define DELETEINBOX "/removeinbox"
#define DELETEOUTBOX "/removeoutbox"
#define STOPBACKEND "/stopbackend"
#define STARTBACKEND "/startbackend"
#define OK "200"
#define REDIRECT "307"
#define SEEOTHER "303"
#define NOTFOUND "404"
#define FOLDER "https://image.flaticon.com/icons/svg/149/149325.svg"
#define TEXT "https://image.flaticon.com/icons/svg/29/29076.svg"
#define PDF "https://image.flaticon.com/icons/svg/29/29587.svg"
#define JPEG "https://image.flaticon.com/icons/svg/29/29513.svg"
#define PNG "https://image.flaticon.com/icons/svg/29/29064.svg"

// function
string find_filename(vector<char>* message);
vector<char> find_file_content(vector<char>* message);
void store_file(string dir, string filename, vector<char> content);
string read_files(vector<file_info*> names);
string parse_type(string filename);
bool is_logged_in(unordered_map<string, string> cookies);
void parse_email(vector<char>* message, string& recipent, string& content,
		string& subject);
char convert_hex_to_char(string hex_char);
string parse_folder(vector<file_info*> folders, string filename, string href);
string parse_path(vector<string> paths);
string parse_server();
void read_servers_from_file(string filename);
void check_state(int index);
string parse_path(string path);
string read_all_emails(vector<email> emails, int type);
void parse_header(string header, string& sender, string& rcpt, string& title, string& time);
string parse_command(int type, string hash, int id);
int parse_mailid(string path);
string for_display(const string& origin);

// global variable
vector<string> filenames = { string("test.txt"), string("cat.jpeg"), string(
		"c.pdf"), string("folder1"), string("dog.png"), string("face.pdf"),
		string("word.txt"), string("folder2") };
unordered_map<string, string> sessions;
vector<string> folders = { string("folder1"), string("folder2"), string(
		"folder3"), string("folder4"), string("folder5"), string("folder6"),
		string("folder7") };
vector<string> paths = { string("Storage"), string("a"), string("b"), string(
		"c") };
string username;
deque<Server> servers;
pthread_mutex_t lock1;
vector<string> server_info;

unordered_map<string, Storage*> all_storages;
unordered_map<string, vector<email>> inbox;
unordered_map<string, vector<email>> outbox;


/*
 * constructor of response and send back reponse to the client
 */
response::response(int client_fd, request req) {
	string path = parse_path(req.path);
	this->version = req.version.substr(0, 8); // skip \r\n
	if (strcasecmp(req.method.c_str(), "get") == 0) {
		// handle get request
		handle_get(path, client_fd, req.cookies);
		delete(req.message);
	} else if (strcasecmp(req.method.c_str(), "post") == 0) {
		// handle post request
		this->status = OK;
		handle_post(path, client_fd, req.message, req.cookies);
		delete(req.message);
	}
}

/*
 * handle get request from client
 */
void response::handle_get(string path, int client_fd,
		unordered_map<string, string> cookies) {
	if (strcasecmp(path.c_str(), HOME) == 0) {
		// Home page
		this->status = OK;
		if (is_logged_in(cookies)) {
			// has logged in
			string address("./html/signin_successful.html");
			string type("text/html");
			this->header["Content-type"] = type;
			this->header["Set-Cookie"] = string("username=") + username;
			unordered_map<string, string> replacement;
			replacement[string("$user")] = username;
			replace_reply(address, type, client_fd, replacement);
		} else {
			string address("./html/signin.html");
			string type("text/html");
			this->header["Content-type"] = type;
			reply(address, type, client_fd);
		}
	} else if (strcasecmp(path.c_str(), SIGNIN_CSS) == 0) {
		// pass css for the home page to the browser
		string address("./html/css/signin.css");
		string type("text/css");
		this->header["Content-type"] = type;
		this->status = OK;
		reply(address, type, client_fd);
	} else if (strcasecmp(path.c_str(), NEWEMAIL) == 0) {
		if (!is_logged_in(cookies)) {
			// not log in
			string address("./html/signin.html");
			string type("text/html");
			this->header["Content-type"] = type;
			this->header["Location"] = string("/");
			this->status = REDIRECT;
			reply(address, type, client_fd);
		} else {
			// email page
			string address("./html/new_email.html");
			string type("text/html");
			this->header["Content-type"] = type;
			unordered_map<string, string> replacement;
			replacement[string("$user")] = username;
			replacement[string("$email")] = string("");
			replacement[string("$subject")] = string("");
			replacement[string("$content")] = string("");
			this->header["Set-Cookie"] = string("username=") + username;
			this->status = OK;
			replace_reply(address, type, client_fd, replacement);
		}
	} else if (strcasecmp(path.c_str(), FILES) == 0) {
		// file page
		if (!is_logged_in(cookies)) {
			string address("./html/signin.html");
			string type("text/html");
			this->header["Content-type"] = type;
			this->status = REDIRECT;
			this->header["Location"] = string("/");
			reply(address, type, client_fd);
		} else {
			string address("./html/files.html");
			string type("text/html");
			this->header["Content-type"] = type;

			Storage* temp = all_storages[username];
			vector<file_info*> filess =
					*(all_storages[username]->refresh_upload());
			string names = read_files(filess);
            vector<file_info*> path_info = all_storages[username]->path;
            vector<string> paths;
            for(int i = 0; i < path_info.size(); i++){
            	paths.push_back(path_info[i]->name);
            }
			string cur_path = parse_path(paths);
			unordered_map<string, string> replacement;
			replacement[string("$list_files")] = names;
			replacement[string("$user")] = username;
			replacement[string("$path")] = cur_path;
			this->header["Set-Cookie"] = string("username=") + username;
			this->status = OK;
			replace_reply(address, type, client_fd, replacement);
		}
	} else if (strcasecmp(path.c_str(), UPLOAD) == 0) {
		if (!is_logged_in(cookies)) {
			string address("./html/signin.html");
			string type("text/html");
			this->header["Content-type"] = type;
			this->status = REDIRECT;
			this->header["Location"] = string("/");
			reply(address, type, client_fd);
		} else {
			// upload page
			string address("./html/upload.html");
			string type("text/html");
			this->header["Content-type"] = type;
			unordered_map<string, string> replacement;
			this->header["Set-Cookie"] = string("username=") + username;
			replacement[string("$user")] = username;
			this->status = OK;
			replace_reply(address, type, client_fd, replacement);
		}
	} else if (path.size() >= 10
			&& strcasecmp(path.substr(0, 10).c_str(), DOWNLOAD) == 0) {
		// download files
		string filename = path.substr(10, path.size());
		string address("./Files/");
		address += filename;
		string type = parse_type(filename);
		string disposition = string("attachment;") + string("filename=\"")
				+ filename + string("\"");
		this->status = OK;
		reply_file(filename, type, disposition, client_fd);
	} else if (path.size() >= 9
			&& strcasecmp(path.substr(0, 9).c_str(), MOVEFILE) == 0) {
		// move file
		string address("./html/move.html");
		string type("text/html");
		this->header["Content-type"] = type;
		string filename = path.substr(10, path.size() - 10);
		unordered_map<string, string> replacement;
		string href("/filemove");
		replacement[string("$user")] = username;
		vector<file_info*> folderss = *(all_storages[username]->get_accessible_movingPath(filename));
		replacement[string("$folders")] = parse_folder(folderss, filename, href);
		this->status = OK;
		replace_reply(address, type, client_fd, replacement);
	} else if (path.size() >= 11
			&& strcasecmp(path.substr(0, 11).c_str(), MOVEFOLDER) == 0) {
		// move file
		string address("./html/move.html");
		string type("text/html");
		this->header["Content-type"] = type;
		string filename = path.substr(12, path.size() - 12);
		string href("/foldermove");
		unordered_map<string, string> replacement;
		replacement[string("$user")] = username;
		vector<file_info*> folderss = *(all_storages[username]->get_accessible_movingPath(filename));
		replacement[string("$folders")] = parse_folder(folderss, filename, href);
		this->status = OK;
		replace_reply(address, type, client_fd, replacement);
	} else if (strcasecmp(path.c_str(), LOGOUT) == 0) {
		// logout request
		sessions.erase(username); // delete the session
		string address("./html/signin.html");
		string type("text/html");
		this->header["Content-type"] = type;
		this->status = REDIRECT;
		this->header["Location"] = string("/");
		reply(address, type, client_fd);
		delete all_storages[username];
		all_storages.erase(username);
	} else if (strcasecmp(path.c_str(), SIGNUP) == 0) {
		// sign up page
		if (is_logged_in(cookies)) {
			this->status = REDIRECT;
			// if the user has already logged in
			// has logged in redirect to home page
			string address("./html/signin_successful.html");
			string type("text/html");
			this->header["Content-type"] = type;
			this->header["Set-Cookie"] = string("username=") + username;
			this->header["Location"] = string("/");
			unordered_map<string, string> replacement;
			replacement[string("$user")] = username;
			replace_reply(address, type, client_fd, replacement);
		} else {
			// show the sign up page
			string address("./html/signup.html");
			string type("text/html");
			this->header["Content-type"] = type;
			reply(address, type, client_fd);
		}
	} else if (path.size() >= 9
			&& strcasecmp(path.substr(0, 9).c_str(), MOVEFILECHOOSEFOLDER)
					== 0) {
		// move folder the the chosen folder
		handle_move_file(path, client_fd, true);
	} else if (path.size() >= 11
			&& strcasecmp(path.substr(0, 11).c_str(), MOVEFOLDERCHOOSEFOLDER)
					== 0) {
		// move folder the the chosen folder
		handle_move_file(path, client_fd, false);
	} else if (path.size() >= 5
			&& strcasecmp(path.substr(0, 5).c_str(), BACK) == 0) {
		// back to specific folder
		handle_back(path, client_fd);
	} else if (strcasecmp(path.c_str(), ADMIN) == 0) {
		// admin page
		servers.clear();
		server_info.clear();
		string servers_html = parse_server();
		string address("./html/admin.html");
		string type("text/html");
		this->header["Content-type"] = type;
		unordered_map<string, string> replacement;
		replacement[string("$servers")] = servers_html;
		this->status = OK;
		replace_reply(address, type, client_fd, replacement);
	} else if (path.size() >= 11
			&& strcasecmp(path.substr(0, 11).c_str(), STOPSERVER) == 0) {
		// stop the given server
		handle_stop_server(path, client_fd);
	} else if (strcasecmp(path.c_str(), KILL) == 0) {
		// kill the current server
		raise(SIGINT);
	} else if (path.size() >= 12
			&& strcasecmp(path.substr(0, 12).c_str(), STARTSERVER) == 0) {
		// start the server
		handle_start_server(path, client_fd);
	} else if (path.size() >= 7 &&
			strcasecmp(path.substr(0, 7).c_str(), DELETE) == 0) {
		// delete file or folder
		handle_delete(path, client_fd);
	} else if (path.size() >= 8 &&
			strcasecmp(path.substr(0, 8).c_str(), OPENDIR) == 0) {
		// open directory
		handle_open_directory(path, client_fd);
	} else if (strcasecmp(path.c_str(), EMAIL) == 0) {
		// shwo email homepage
		if (is_logged_in(cookies)) {
			// has logged in
			string address("./html/email.html");
			string type("text/html");
			this->header["Content-type"] = type;
			this->header["Set-Cookie"] = string("username=") + username;
			unordered_map<string, string> replacement;

			string master = "127.0.0.1:10000";
			vector<email> in_emails = recvfrom_backend(master, username, true);
			vector<email> out_emails = recvfrom_backend(master, username, false);
			inbox.erase(username);
			outbox.erase(username);
			inbox.insert({username, in_emails});
			outbox.insert({username, out_emails});

			string inbox_emails = read_all_emails(in_emails, 0);
			string sent_emails = read_all_emails(out_emails, 1);
//			string trash_emails = read_all_emails(trash, 2);
			replacement[string("$user")] = username;
			replacement[string("$inbox")] = inbox_emails;
			replacement[string("$sent")] = sent_emails;
			replacement[string("$trash")] = string("");
			replace_reply(address, type, client_fd, replacement);
		} else {
			string address("./html/signin.html");
			string type("text/html");
			this->header["Content-type"] = type;
			reply(address, type, client_fd);
		}
	} else if (path.size() >= 10 &&
			strcasecmp(path.substr(0, 10).c_str(), SHOWINBOX) == 0) {
		int id = parse_mailid(path);
		string master = "127.0.0.1:10000";
		string hash = inbox[username].at(id).hash;
		string command_list = parse_command(0, hash, id);
		string sender, rcpt, title, time;
		unordered_map<string, string> replacement;
		parse_header(inbox[username].at(id).header, sender, rcpt, title, time);
		replacement[string("$user")] = username;
		replacement[string("$Action")] = string("From");
		replacement[string("$sender")] = sender;
		replacement[string("$time")] = time;
		replacement[string("$Subject")] = title;
		replacement[string("$content")] = for_display(inbox[username].at(id).content);
		replacement[string("$command-list")] = command_list;
		string address("./html/showemail.html");
		string type("text/html");
		this->header["Content-type"] = type;
		this->header["Set-Cookie"] = string("username=") + username;
		replace_reply(address, type, client_fd, replacement);
	} else if (path.size() >= 9 &&
			strcasecmp(path.substr(0, 9).c_str(), SHOWSENT) == 0) {
		int id = parse_mailid(path);
		string hash = outbox[username].at(id).hash;
		string command_list = parse_command(1, hash, id);
		string sender, rcpt, title, time;
		unordered_map<string, string> replacement;
		parse_header(outbox[username].at(id).header, sender, rcpt, title, time);
		replacement[string("$user")] = username;
		replacement[string("$Action")] = string("To");
		replacement[string("$sender")] = rcpt;
		replacement[string("$time")] = time;
		replacement[string("$Subject")] = title;
		replacement[string("$content")] = for_display(outbox[username].at(id).content);
		replacement[string("$command-list")] = command_list;
		string address("./html/showemail.html");
		string type("text/html");
		this->header["Content-type"] = type;
		this->header["Set-Cookie"] = string("username=") + username;
		replace_reply(address, type, client_fd, replacement);
	} else if (path.size() >= 6 &&
			strcasecmp(path.substr(0, 6).c_str(), REPLY) == 0) {
		int id = parse_mailid(path);
		string address("./html/new_email.html");
		string type("text/html");
		this->header["Content-type"] = type;
		unordered_map<string, string> replacement;
		replacement[string("$user")] = username;
		string sender, rcpt, title, time;
		parse_header(inbox[username].at(id).header, sender, rcpt, title, time);
		replacement[string("$email")] = sender;
		replacement[string("$subject")] = string("Re: ") + title;
		replacement[string("$content")] = "From: " + sender + "\r\nTo: " + rcpt + "\r\nDate: " + time + "\r\n" + inbox[username].at(id).content;
		this->header["Set-Cookie"] = string("username=") + username;
		this->status = OK;
		replace_reply(address, type, client_fd, replacement);
	} else if (path.size() >= 8 &&
			strcasecmp(path.substr(0, 8).c_str(), FORWARD) == 0) {
		int id = parse_mailid(path);
		string address("./html/new_email.html");
		string type("text/html");
		this->header["Content-type"] = type;
		unordered_map<string, string> replacement;
		replacement[string("$user")] = username;
		string sender, rcpt, title, time;
		parse_header(inbox[username].at(id).header, sender, rcpt, title, time);
		replacement[string("$email")] = string("");
		replacement[string("$subject")] = string("Fwd: ") + title;

		replacement[string("$content")] = "From: " + sender + "\r\nTo: " + rcpt + "\r\nDate: " + time + "\r\n" + inbox[username].at(id).content;
		this->header["Set-Cookie"] = string("username=") + username;
		this->status = OK;
		replace_reply(address, type, client_fd, replacement);
	} else if (path.size() >= 12 &&
			strcasecmp(path.substr(0, 12).c_str(), DELETEINBOX) == 0) {
//		int id = parse_mailid(path);

		// TODO
		// delete the email in inbox
		string master = "127.0.0.1:10000";
		set<string> delete_hash;
		size_t found = path.find("=");
		string hash = path.substr(found+1); hash.pop_back(); // remove ? at last
		delete_hash.insert(hash);
		update_backend(master, username, delete_hash, true);

		// redirect to email
		string address("./html/email.html");
		string type("text/html");
		this->status = SEEOTHER;
		this->header["Content-type"] = type;
		this->header["Location"] = string("/email");
		unordered_map<string, string> replacement;
		replacement[string("$user")] = username;
		this->header["Set-Cookie"] = string("username=") + username;
		replace_reply(address, type, client_fd, replacement);
	} else if (path.size() >= 12 &&
			strcasecmp(path.substr(0, 13).c_str(), DELETEOUTBOX) == 0) {
//		int id = parse_mailid(path);

		// TODO
		// delete the email in outbox
		string master = "127.0.0.1:10000";
		set<string> delete_hash;
		size_t found = path.find("=");
		string hash = path.substr(found+1); hash.pop_back(); // remove ? at last
		delete_hash.insert(hash);
		update_backend(master, username, delete_hash, false);

		// redirect to email
		string address("./html/email.html");
		string type("text/html");
		this->status = SEEOTHER;
		this->header["Content-type"] = type;
		this->header["Location"] = string("/email");
		unordered_map<string, string> replacement;
		replacement[string("$user")] = username;
		this->header["Set-Cookie"] = string("username=") + username;
		replace_reply(address, type, client_fd, replacement);
	} else if (path.size() >= 12 &&
			strcasecmp(path.substr(0, 12).c_str(), STOPBACKEND) == 0) {
		// stop backend
		handle_stop_backend(path, client_fd);
	} else if (path.size() >= 13 &&
			strcasecmp(path.substr(0, 13).c_str(), STARTBACKEND) == 0) {
		// stop backend
		handle_start_backend(path, client_fd);
	}
}

/*
 * start backend server
 */
void response::handle_start_backend(string path, int client_fd) {
	// parse the path
	int id = 0;
	int index = 0;
	while (path[index] != '=')
		index++;
	index++;
	while (index < path.size()) {
		id = id * 10 + path[index] - '0';
		index++;
	}
	StateCheck* sc = new StateCheck();
	vector<string> cur_server = sc->parse_id(server_info[id]);
	string nodeID = cur_server[0];
	string repID = cur_server[1];
	size_t pos = cur_server[2].find(":");
	string ip = cur_server[2].substr(0, pos);
	string port = cur_server[2].substr(pos + 1);

	// start the server
	int count = 0;
	while (true) {
		pid_t pid = fork();
		if (pid == 0) {
			if (count > 0) {
				continue;
			}
			count++;
			// child process
			char* argv[8];
			argv[0] = (char*) "../kvstore/dataserver";
			argv[1] = (char*) "-r";
			argv[2] = (char*) "-p";
			string s = "../kvstore/node/" + nodeID + "/" + repID;
			argv[3] = (char*) s.c_str();
			argv[4] = (char*) "../kvstore/serverlist.txt";
			argv[5] = (char*)nodeID.c_str();
			argv[6] = (char*)repID.c_str();
			argv[7] = NULL;
			execv(argv[0], argv);
		} else {
			sleep(1);
			// redirect to admin page
			// redirect to admin
			string servers_html = parse_server();
			string address("./html/admin.html");
			string type("text/html");
			this->header["Content-type"] = type;
			this->header["Location"] = string("http://localhost:8080/admin");
			unordered_map<string, string> replacement;
			this->status = REDIRECT;
			replace_reply(address, type, client_fd, replacement);
			break;
		}

	}
}

/*
 * stop the backend server
 */
void response::handle_stop_backend(string path, int client_fd) {
	// parse the path and get the id
	int id = 0;
	int index = 0;
	while (path[index] != '=')
		index++;
	index++;
	while (index < path.size()) {
		id = id * 10 + path[index] - '0';
		index++;
	}
	StateCheck* sc = new StateCheck();
	vector<string> cur_server = sc->parse_id(server_info[id]);
	size_t pos = cur_server[2].find(":");
	string ip = cur_server[2].substr(0, pos);
	string port = cur_server[2].substr(pos + 1);

	int sock = socket(PF_INET, SOCK_STREAM, 0);
	// build a socket and connect to the given server
	struct sockaddr_in dest;
	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(atoi(port.c_str()));
	inet_pton(AF_INET, ip.c_str(), &(dest.sin_addr));
	connect(sock, (sockaddr*) &dest, sizeof(dest));
	string buffer("closeall\r\n");
	write(sock,buffer.c_str(),buffer.length());
	//send(sock, buffer.c_str(), buffer.size(), 0);
	close(sock);

	sleep(1);
	// redirect to admin
	servers.clear();
	string address("./html/admin.html");
	string type("text/html");
	this->header["Content-type"] = type;
	this->header["Location"] = string("/admin");
	unordered_map<string, string> replacement;
	this->status = REDIRECT;
	replace_reply(address, type, client_fd, replacement);
}

/*
 * open the folder
 */
void response::handle_open_directory(string path, int client_fd) {
	// parse the forldername
	string foldername;
	int index = 0;
	while (path[index] != '?') index++;
	index++;
	while (index < path.size()) foldername.push_back(path[index++]);

	// open folder
	// TODO
    all_storages[username]->open_folder(username,foldername);
	// redirect to file page
	string address("./html/files.html");
	string type("text/html");
	this->status = SEEOTHER;
	this->header["Content-type"] = type;
	this->header["Location"] = string("/files");
	unordered_map<string, string> replacement;
	replacement[string("$user")] = username;
	this->header["Set-Cookie"] = string("username=") + username;
	replace_reply(address, type, client_fd, replacement);
}

/*
 * delete the file or folder
 */
void response::handle_delete(string path, int client_fd) {
	// parse the filename
	int index = 0;
	string filename;
	while (path[index] != '?') {
		index++;
	}
	index++;
	while (index < path.size()) {
		filename.push_back(path[index++]);
	}

	// delete
	// TODO
	string pathID = all_storages[username]->path.at(all_storages[username]->path.size()-1)->id;
	bool delete_try = all_storages[username]->delete_file(username,pathID,filename);

	// redirect to file
	string address("./html/files.html");
	string type("text/html");
	this->status = SEEOTHER;
	this->header["Content-type"] = type;
	this->header["Location"] = string("/files");
	unordered_map<string, string> replacement;
	replacement[string("$user")] = username;
	this->header["Set-Cookie"] = string("username=") + username;
	replace_reply(address, type, client_fd, replacement);
}

/*
 * start the chosen server
 */
void response::handle_start_server(string path, int client_fd) {
	// parse the path
	int id = 0;
	int index = 0;
	while (path[index] != '=')
		index++;
	index++;
	while (index < path.size()) {
		id = id * 10 + path[index] - '0';
		index++;
	}

	// start the server
	int count = 0;
	while (true) {
		pid_t pid = fork();
		if (pid == 0) {
			if (count > 0) {
				continue;
			}
			count++;
			cout << to_string(servers.at(id).port) << endl;
			// child process
			char* argv[5];
			argv[0] = (char*) "./frontend";
			argv[1] = (char*) "-v";
			argv[2] = (char*) "-p";
			argv[3] = (char*) (to_string(servers.at(id).port)).c_str();
			argv[4] = NULL;
			execv(argv[0], argv);
		} else {
			cout << to_string(pid) << endl;
			sleep(1);
			// redirect to admin page
			// redirect to admin
			servers.clear();
			string servers_html = parse_server();
			string address("./html/admin.html");
			string type("text/html");
			this->header["Content-type"] = type;
			this->header["Location"] = string("http://localhost:8080/admin");
			unordered_map<string, string> replacement;
			this->status = REDIRECT;
			replace_reply(address, type, client_fd, replacement);
			break;
		}

	}

}

/*
 * stop the choosen server
 */
void response::handle_stop_server(string path, int client_fd) {
	// parse the path and get the id
	int id = 0;
	int index = 0;
	while (path[index] != '=')
		index++;
	index++;
	while (index < path.size()) {
		id = id * 10 + path[index] - '0';
		index++;
	}

	int sock = socket(PF_INET, SOCK_STREAM, 0);
	// build a socket and connect to the given server
	struct sockaddr_in dest;
	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(servers.at(id).port);
	inet_pton(AF_INET, servers.at(id).address.c_str(), &(dest.sin_addr));
	cout << servers.at(id).port << endl;
	connect(sock, (sockaddr*) &dest, sizeof(dest));
	string buffer("GET /kill HTTP/1.1\r\n\r\n");
	send(sock, buffer.c_str(), buffer.size(), 0);
	close(sock);

	sleep(1);
	// redirect to admin
	servers.clear();
	string servers_html = parse_server();
	string address("./html/admin.html");
	string type("text/html");
	this->header["Content-type"] = type;
	this->header["Location"] = string("/admin");
	unordered_map<string, string> replacement;
	this->status = REDIRECT;
	replace_reply(address, type, client_fd, replacement);
}

/*
 * go back to the chosen folder
 */
void response::handle_back(string path, int client_fd) {
	// parse the path get the index
	int index = 0;
	while (path[index] != '=')
		index++;
	index++;
	int id = 0;
	while (index < path.size())
		id += id * 10 + path[index++] - '0';

	// refresh the page
	// TODO
    all_storages[username]->goBack(id);

	// redirect to the file page
	string address("./html/files.html");
	string type("text/html");
	this->status = SEEOTHER;
	this->header["Content-type"] = type;
	this->header["Location"] = string("/files");
	unordered_map<string, string> replacement;
	replacement[string("$user")] = username;
	this->header["Set-Cookie"] = string("username=") + username;
	replace_reply(address, type, client_fd, replacement);
}

/*
 * move the file to the given folder
 */
void response::handle_move_file(string path, int client_fd, bool isFile) {
	// parse filename and the folder name
	string info;
	if (isFile) {
		info = path.substr(22, path.size() - 22);
	} else {
		info = path.substr(24, path.size() - 24);
	}
	string filename;
	string folder;
	int index = 0;
	while (info[index] != '=')
		index++;
	index++;
	while (info[index] != '&') {
		filename.push_back(info[index++]);
	}
	while (info[index] != '=')
		index++;
	index++;
	while (index < info.size()) {
		folder.push_back(info[index++]);
	}

	// move the file
	// TODO
	bool move_file_try = all_storages[username]->move_file(folder,filename);

	// redirect to the file page
	string address("./html/files.html");
	string type("text/html");
	this->status = SEEOTHER;
	this->header["Content-type"] = type;
	this->header["Location"] = string("/files");
	unordered_map<string, string> replacement;
	replacement[string("$user")] = username;
	this->header["Set-Cookie"] = string("username=") + username;
	replace_reply(address, type, client_fd, replacement);
}

/*
 * reply the file to the client
 */
void response::reply_file(string filename, string type, string disposition,
		int client_fd) {
	string server_response(this->version);
	server_response += " " + this->status + " " + "OK" + "\r\n";
	server_response += string("Content-type: ") + string(type) + string("\r\n");
	server_response += string("Content-Disposition: ") + disposition
			+ string("\r\n");
	server_response += "\r\n";
	// convert the string to char vector
	vector<char> res(server_response.begin(), server_response.end());

	vector<char> content;
	//all_storages[username]->refresh_upload();
	cout<<"size issssssssssssssssssssssssss:"<<all_storages[username]->files_in_path.size()<<endl;
		bool download_status = all_storages[username]->download_file(username,filename ,content);
		for(int i =0; i < content.size(); i++){
			res.push_back(content[i]);
		}
	printf("vector size: %lu\n", res.size());

	// read the character into the vector
	//read_file_into_vector(res, address);
	printf("vector size: %lu\n", res.size());

	do_write(client_fd, &res[0], res.size());
}

/*
 * handle post request from client
 */
void response::handle_post(string path, int client_fd, vector<char>* message,
		unordered_map<string, string> cookies) {
	if (strcasecmp(path.c_str(), HOME) == 0) {
		// handle sign in request
		handle_signin(path, client_fd, message, cookies);
	} else if (strcasecmp(path.c_str(), UPLOAD) == 0) {
		// handle upload file
		handle_upload(path, client_fd, message);
	} else if (strcasecmp(path.c_str(), NEWEMAIL) == 0) {
		// email
		handle_send_email(path, client_fd, message);
	} else if (strcasecmp(path.c_str(), SIGNUP) == 0) {
		// sign up
		handle_signup(path, client_fd, message);
	} else if (strcasecmp(path.c_str(), CREATEFOLDER) == 0) {
		// create new folder
		handle_create_newfolder(path, client_fd, message);
	} else if (strcasecmp(path.c_str(), RENAME) == 0) {
		// rename the file and folder
		handle_rename(path, client_fd, message);
	}
}

/*
 * handle rename the file
 */
void response::handle_rename(string path, int client_fd,
		vector<char>* message) {
	// parse new name and previous from the message
	string info(message->begin(), message->end());
	string prename;
	string newname;
	int index = 0;
	while (info[index] != '=')
		index++;
	index++;
	while (info[index] != '&')
		newname.push_back(info[index++]);
	index++;
	while (info[index] != '=')
		index++;
	index++;
	while (index < info.size())
		prename.push_back(info[index++]);

	cout << prename << endl;
	cout << newname << endl;


	bool can_rename = false;
	//string pathID = all_storages[username]->path.at(all_storages[username]->path.size() -1)->id;
	//string fileID = all_storages[username]->files_in_path(prename)
    can_rename = all_storages[username]->rename_file(username,prename,newname);
	if (can_rename) {
		// return OK status code
		string address("./html/files.html");
		string type("text/html");
		this->status = OK;
		this->header["Content-type"] = type;
		vector<file_info*>* filess = all_storages[username]->refresh_upload();
		string names = read_files(*filess);
		unordered_map<string, string> replacement;
		replacement[string("$user")] = username;
		replace_reply(address, type, client_fd, replacement);
	} else {
		// return other status code
		string address("./html/files.html");
		string type("text/html");
		this->status = REDIRECT;
		this->header["Content-type"] = type;

		//string names = read_files(filenames);
		unordered_map<string, string> replacement;
		replacement[string("$user")] = username;
		replace_reply(address, type, client_fd, replacement);
	}
}

/*
 * handle request to create new folder
 */
void response::handle_create_newfolder(string path, int client_fd,
		vector<char>* message) {
	// parse the folder name
	string info = string(message->begin(), message->end());
	string foldername = info.substr(11, info.size() - 11);
	cout << foldername << endl;

	// create new folder
	// TODO
	string pathID = all_storages[username]->path.at(all_storages[username]->path.size()-1)->id;
	bool create_try = all_storages[username]->create_folder(username,pathID,foldername);
    if(create_try){
    	this->status = OK;
    }else {
    	this->status = REDIRECT;
    }
	// redirect to the same file page
	string address("./html/files.html");
	string type("text/html");
	//this->status = REDIRECT;
	this->header["Content-type"] = type;
	vector<file_info*>* filess = all_storages[username]->refresh_upload();
	string names = read_files(*filess);
	//string names = read_files(filenames);
	unordered_map<string, string> replacement;
	replacement[string("$list_files")] = names;
	replacement[string("$user")] = username;
	this->header["Set-Cookie"] = string("username=") + username;
	replace_reply(address, type, client_fd, replacement);
}

/*
 * handle user sign up
 */
void response::handle_signup(string path, int client_fd,
		vector<char>* message) {
	// check whether the user has already logged in
	// parse the message to get username and password
	char copy[256];
	string info(message->begin(), message->end());
	strcpy(copy, info.c_str());
	char *token;
	token = strtok(copy, "&");

	// get username
	char *subToken;
	char copy2[256];
	strcpy(copy2, token);
	token = strtok(NULL, " &");
	subToken = strtok(copy2, "=");
	subToken = strtok(NULL, "=");
	username = string(subToken);

	// get password
	char copy3[256];
	strcpy(copy3, token);
	char *subToken2;
	subToken2 = strtok(copy3, "=");
	subToken2 = strtok(NULL, "=");
	string password(subToken2);

	bool can_signup = false;
	// check whether the username and the password are valid
	// TODO
	Storage* s = new Storage(username);
	can_signup = s->resgister(username, password);

	if (can_signup) {
		// add new seesion and storage
		sessions[username] = get_time();
		// TODO
		all_storages.insert(make_pair(username, s));
		// reply to the client
		string address("./html/signin_successful.html");
		string type("text/html");
		this->header["Content-type"] = type;
		this->header["Set-Cookie"] = string("username=") + username;
		unordered_map<string, string> replacement;
		replacement[string("$user")] = username;
		replace_reply(address, type, client_fd, replacement);
	} else {
		// show wrong message
		delete s;
		string address("./html/signup_failed.html");
		string type("text/html");
		this->header["Content-type"] = type;
		reply(address, type, client_fd);
	}
}

/*
 * handle send email function
 */
void response::handle_send_email(string path, int client_fd,
		vector<char>* message) {
	string recipient;
	string content;
	string subject;
	parse_email(message, recipient, content, subject);
	string sender = username + string("@localhost");
	printf("recipent: %s\n", recipient.c_str());
	printf("content: %s\n", content.c_str());
	printf("sender: %s\n", sender.c_str());
	printf("subject: %s\n", subject.c_str());

	// send email
	string smtp = "127.0.0.1:7000";
	set<string> rcpts; rcpts.insert(recipient);
	content = "Subject: " + subject + "\r\n" + content + "\r\n";
	sendto_smpt_server(smtp, sender, rcpts, content);

	// redirect to email page
	string address("./html/email.html");
	string type("text/html");
	this->status = SEEOTHER;
	this->header["Content-type"] = type;
	this->header["Location"] = string("/email");
	unordered_map<string, string> replacement;
	replacement[string("$user")] = username;
	this->header["Set-Cookie"] = string("username=") + username;
	replace_reply(address, type, client_fd, replacement);
}

/*
 * handle upload request
 */
void response::handle_upload(string path, int client_fd,
		vector<char>* message) {
	// find filename
	string filename = find_filename(message);

	printf("filename: %s\n", filename.c_str());
	printf("message size: %lu\n", message->size());

	// find content
	vector<char> content = find_file_content(message);
	string content_size;
	if (content.size() < 1024) {
		// smaller than KB
		content_size += to_string(content.size()) + "B";
	} else if (content.size() < 1000000) {
		// smaller than MB
		content_size += to_string(content.size() / 1024) + "KB";
	} else {
		// bigger than MB
		double size = content.size() / 900000;
		char size_str[20];
		sprintf(size_str, "%.2f",size);
		content_size = string(size_str) + "MB";
		//content_size += to_string(content.size() / 1000000) + "MB";
	}

	// store the file
	string dir("./Test");
	store_file(dir, filename, content);

	// redirect to file page
	string address("./html/files.html");
	string type("text/html");
	this->status = SEEOTHER;
	this->header["Content-type"] = type;
	this->header["Location"] = string("/files");

	vector<file_info*> current_path = all_storages[username]->path;
	file_info* current = current_path.at(current_path.size() - 1);


	all_storages[username]->upload_file(username, current->id, filename,
			content, content_size);
	//cout<<"size issssssssssssssssssssssssss:"<<all_storages[username]->files_in_path.size()<<endl;
	vector<file_info*> filess = *(all_storages[username]->refresh_upload());
	cout<<"---get the new page-------------"<<endl;
	string names = read_files(filess);

	unordered_map<string, string> replacement;
	replacement[string("$user")] = username;
	this->header["Set-Cookie"] = string("username=") + username;
	replace_reply(address, type, client_fd, replacement);

}

/*
 * handle sign in post request
 */
void response::handle_signin(string path, int client_fd, vector<char>* message,
		unordered_map<string, string> cookies) {
	// check whether the user has already logged in
	bool already_login = false;
	if (is_logged_in(cookies)) {
		already_login = true;
	} else {
		// not logged in
		// parse the message to get username and password
		char copy[256];
		string info(message->begin(), message->end());
		strcpy(copy, info.c_str());
		char *token;
		token = strtok(copy, "&");

		// get username
		char *subToken;
		char copy2[256];
		strcpy(copy2, token);
		token = strtok(NULL, " &");
		subToken = strtok(copy2, "=");
		subToken = strtok(NULL, "=");
		username = string(subToken);

		// get password
		char copy3[256];
		strcpy(copy3, token);
		char *subToken2;
		subToken2 = strtok(copy3, "=");
		subToken2 = strtok(NULL, "=");
		string password(subToken2);

		// check whether the username and the password are valid
		// TODO
		Storage* s = new Storage(username);
		bool check_sign = s->sign_in(username,password);
		if(check_sign){
			all_storages.insert(make_pair(username,s));
			// add new seesion
			printf("session: username: %s\n", username.c_str());
			sessions[username] = get_time();
			already_login = true;
		}

	}
	if (already_login) {
		// reply to the client
		string address("./html/signin_successful.html");
		string type("text/html");
		this->header["Content-type"] = type;
		this->header["Set-Cookie"] = string("username=") + username;
		unordered_map<string, string> replacement;
		replacement[string("$user")] = username;
		replace_reply(address, type, client_fd, replacement);
	} else {
		string address("./html/signin_failed.html");
		string type("text/html");
		this->header["Content-type"] = type;
		reply(address, type, client_fd);
	}
}

/*
 * replace html file with message body and reply to the client
 */
void response::replace_reply(string address, string type, int client_fd,
		unordered_map<string, string> replacement) {
	string server_response(this->version);
	string status_char;
	if (this->status == OK) {
		status_char = string("OK");
	} else if (this->status == REDIRECT) {
		status_char = string("Temporary Redirect");
	}
	server_response += " " + this->status + " " + status_char + "\r\n";
	for (auto iter = this->header.begin(); iter != this->header.end(); iter++) {
		server_response += iter->first;
		server_response += string(":");
		server_response += iter->second;
		server_response += string("\r\n");
	}
	server_response += "\r\n";
	server_response += replace_get_html(address, replacement);
	vector<char> res(server_response.begin(), server_response.end());
	do_write(client_fd, &res[0], strlen(server_response.c_str()));
}

/*
 * reply the corresponding file to the client
 */
void response::reply(string address, string type, int client_fd) {
	string server_response(this->version);
	string status_char;
	if (this->status == OK) {
		status_char = string("OK");
	} else if (this->status == REDIRECT) {
		status_char = string("Temporary Redirect");
	} else if (this->status == SEEOTHER) {
		status_char = string("See Other");
	}
	server_response += " " + this->status + " " + status_char + "\r\n";
	for (auto iter = this->header.begin(); iter != this->header.end(); iter++) {
		server_response += iter->first;
		server_response += string(": ");
		server_response += iter->second;
		server_response += string("\r\n");
	}
	server_response += "\r\n";
	server_response += getHtml(address);
	char res[10000];
	strcpy(res, server_response.c_str());
	do_write(client_fd, res, strlen(server_response.c_str()));
}

/*
 * check whether the user has already logged in using cookies
 */
bool is_logged_in(unordered_map<string, string> cookies) {
	if (cookies.find(string("username")) != cookies.end()) {
		// find the username
		if (sessions.find(cookies[string("username")]) != sessions.end()) {
			// if the username is in the session
			username = cookies[string("username")];
			return true;
		}
	}

	return false;
}

/*
 * parse the files in the directory to html li elements
 */
string read_files(vector<file_info*> names) {
	string res;
	for (int i = 0; i < names.size(); i++) {
		string type;
		if (names[i]->is_file) {
					type = parse_type(names.at(i)->name);
				} else {
					type = "folder";
				}
		if (i % 6 == 0) {
			res += string("<div class=\"row\" style=\"margin-top: 3px;\">");
		}
		res += string("<div class=\"col-2\">");
		if (type != string("folder")) {
			res += string("<a href=\"/download?");
		} else {
			res += string("<a href=\"/opendir?");
		}
		res += names.at(i)->name;
		res += string("\">");
		res +=
				string(
						"<div style=\"width:80px; font-size:80%; text-align:center;\">");
		res += string("<img src=\"");
		// add type image
		if (type == string("image/jpeg")) {
			res += string(JPEG);
		} else if (type == string("image/png")) {
			res += string(PNG);
		} else if (type == string("application/pdf")) {
			res += string(PDF);
		} else if (type == string("text/plain")) {
			res += string(TEXT);
		} else {
			res += string(FOLDER);
		}
		res += string("\" alt=\"alternate text\" width=\"80\" ");
		res += string("height=\"80\" style=\"padding-bottom:0.5em;\"/>");
		res += names.at(i)->name;
		res += string("</a>");
		if (type != string("folder")) {
			res += string("<div>size: ");
			res += names.at(i)->size;
			res += string("</div>");
		}
		res += string("<div>time: ");
		res += names.at(i)->time;
		res += string("</div>");
		if (type != string("folder")) {
			res += string("<div><a href=\"download?") + names.at(i)->name
					+ string("\"><button class=\"btn-small btn-primary\"")
					+ string("\">Download</button></a></div>");
		}
		res +=
				string(
						"<div><button class=\"btn-small btn-primary\" onclick=\"Rename('");
		res += names.at(i)->name;
		res += string("')\" style=\"margin-top: 2px;\">Rename</button></div>");
		if (type != string("folder")) {
			res +=
					string("<div><a href=\"movefile?") + names.at(i)->name
							+ string(
									"\"><button class=\"btn-small btn-primary\" style=\"margin-top: 2px;\">Move</button></a></div>");
		} else {
			res +=
					string("<div><a href=\"movefolder?") + names.at(i)->name
							+ string(
									"\"><button class=\"btn-small btn-primary\" style=\"margin-top: 2px;\">Move</button></a></div>");
		}

		res +=
				string("<div><a href=\"delete?") + names.at(i)->name
						+ string("\"><button class=\"btn-small btn-danger\" ")
						+ string(
								" style=\"margin-top: 2px;\">Delete</button></a></div>");
		res += string("</div>");
		res += string("</div>");

		// close the row div
		if ((i + 1) % 6 == 0 || i == names.size() - 1) {
			res += string("</div>");
		}
	}

	return res;
}

/*
 * parse all the folders to a string in html
 */
string parse_folder(vector<file_info*> folders, string filename, string href) {
	string res;
	for (int i = 0; i < folders.size(); i++) {
		if (i % 6 == 0) {
			res += string("<div class=\"row\" style=\"margin-top: 3px;\">");
		}
		res += string("<div class=\"col-2\">");
		res += string("<a href=\"") + href + string("choosefolder?move=")
				+ filename + string("&to=") + folders.at(i)->name + string("\">");
		res +=
				string(
						"<div style=\"width:80px; font-size:80%; text-align:center;\">");
		res += string("<img src=\"");
		res += string(FOLDER);
		res += string("\" alt=\"alternate text\" width=\"80\" ");
		res += string("height=\"80\" style=\"padding-bottom:0.5em;\"/>");
		res += folders.at(i)->name;
		res += string("</a>");
		res += string("</div>");
		res += string("</div>");
		// close the row div
		if ((i + 1) % 6 == 0 || i == folders.size() - 1) {
			res += string("</div>");
		}
	}
	return res;
}

/*
 * return the content type of the file
 */
string parse_type(string filename) {
	if (filename.find("jpeg") != filename.npos
			|| filename.find("jpg") != filename.npos) {
		// image
		return string("image/jpeg");
	} else if (filename.find("png") != filename.npos) {
		// png image
		return string("image/png");
	} else if (filename.find("pdf") != filename.npos) {
		// pdf file
		return string("application/pdf");
	} else  {
		// text file
		return string("text/plain");
	}
}

/*
 * return the content of the uploaded file
 */
vector<char> find_file_content(vector<char>* message) {
	vector<char> res;
	vector<char> start_char = { '\r', '\n', '\r', '\n' };
	vector<char> end_char = { '-', '-', '-', '-', '-', '-' };
	// find the position of \r\n and skip them
	auto start = search(message->begin(), message->end(), start_char.begin(),
			start_char.end()) + 4;
	// find the position of ------
	auto end = search(start, message->end(), end_char.begin(), end_char.end())
			- 2; // skip \r\n
	res.insert(res.end(), start, end);
	return res;
}

/*
 * store the file into the given directory
 */
void store_file(string dir, string filename, vector<char> content) {
	printf("content size: %lu\n", content.size());
	string address(dir);
	address += string("/") + filename;
	FILE* fp;
	fp = fopen(address.c_str(), "wb");
	fwrite(&content[0], 1, content.size(), fp);
	fclose(fp);
}

/*
 * return the filename of the uploaded file
 */
string find_filename(vector<char>* message) {
	string tmp(message->begin(), message->end());
	size_t index = tmp.find("filename=", 0);
	// skip the filename
	index += 10;
	// find position of second "
	size_t end_index = tmp.find("\"", index);
	return tmp.substr(index, end_index - index);
}

/*
 * change the hex in the path to character
 */
string parse_path(string pre_path) {
	int pos = 0;
	string res;
	while (pos < pre_path.size()) {
		if (pre_path[pos] != '%') {
			res.push_back(pre_path[pos]);
			pos++;
		} else {
			string hex_char(pre_path.begin() + pos + 1,
					pre_path.begin() + pos + 3);
			char hex_result = convert_hex_to_char(hex_char);
			res.push_back(hex_result);
			pos += 3;
		}
	}

	return res;
}

/*
 * parse the email to standard format
 */
void parse_email(vector<char>* message, string& recipent, string& content,
		string& subject) {
	int index = 0;
	while (message->at(index) != '=')
		index++;
	index++;

	// parse username
	while (message->at(index) != '&') {
		if (message->at(index) == '+') {
			recipent.push_back(' ');
			index++;
		} else if (message->at(index) == '%') {
			string hex_char(message->begin() + index + 1,
					message->begin() + index + 3);
			char hex_result = convert_hex_to_char(hex_char);
			recipent.push_back(hex_result);
			index += 3;
		} else {
			recipent.push_back(message->at(index));
			index++;
		}
	}

	// parse subject
	index += 1;
	while (message->at(index) != '=')
		index++;
	index++;
	while (message->at(index) != '&') {
		if (message->at(index) == '+') {
			subject.push_back(' ');
			index++;
		} else if (message->at(index) == '%') {
			string hex_char(message->begin() + index + 1,
					message->begin() + index + 3);
			char hex_result = convert_hex_to_char(hex_char);
			subject.push_back(hex_result);
			index += 3;
		} else {
			subject.push_back(message->at(index));
			index++;
		}
	}

	//parse message
	index += 1;
	while (message->at(index) != '=')
		index++;
	index++;
	while (index < message->size()) {
		if (message->at(index) == '+') {
			content.push_back(' ');
			index++;
		} else if (message->at(index) == '%') {
			string hex_char(message->begin() + index + 1,
					message->begin() + index + 3);
			char hex_result = convert_hex_to_char(hex_char);
			content.push_back(hex_result);
			index += 3;
		} else {
			content.push_back(message->at(index));
			index++;
		}
	}
}

/*
 * parse the path vector to a string in html
 */
string parse_path(vector<string> path) {
	string res;
	for (int i = 0; i < path.size(); i++) {
		res += string("<a href=\"/back?index=") + to_string(i);
		res +=
				string(
						"\" style=\"font-size: 25px; font-family: 'Gothic A1', sans-serif;\">");
		res += string("/") + path.at(i);
		res += string("</a>");
	}

	return res;
}

/*
 * convert hex string to char
 */
char convert_hex_to_char(string hex_char) {
	stringstream ss;
	unsigned char res;
	unsigned int buf;
	ss << hex << hex_char.substr(0, 2);
	ss >> buf;
	res = static_cast<unsigned char>(buf);
	return res;
}

/*
 * parse all the servers into a table
 */
string parse_server() {
	string res;
	read_servers_from_file("servers.txt");
	for (int i = 0; i < servers.size(); i++) {
		res += string("<tr>");
		res += string("<td>") + to_string(i) + string("</td>");
		res += string("<td>HTTP Server</td>");
		res += string("<td>") + servers.at(i).address + string("</td>");
		res += string("<td>") + to_string(servers.at(i).port) + string("</td>");
		res += string("<td>");
		check_state(i);
		if (servers.at(i).running) {
			res += string("RUNNING</td>");
		} else {
			res += string("DOWN</td>");
		}
		res += string("<td><a href=\"/stopserver?index=") + to_string(i)
				+ string(
						"\">Stop</a><span> <span><a href=\"/startserver?index=")
				+ to_string(i) + string("\">Start</a></td>");
		res += string("</tr>");
	}

	// parse backend server
	StateCheck* sc = new StateCheck();
	string backend_servers = sc->getState("127.0.0.1:10000", "states\r\n");
	server_info = sc->parse_all_server(backend_servers);
	for (int i = 0; i < server_info.size(); i++) {
		vector<string> cur_server = sc->parse_id(server_info[i]);
		res += string("<tr>");
		res += string("<td>") + cur_server[0] + string("-") +
				cur_server[1] + string("</td>");
		res += string("<td>Backend Server</td>");
		size_t pos = cur_server[2].find(":");
		string ip = cur_server[2].substr(0, pos);
		string port = cur_server[2].substr(pos + 1);
		res += string("<td>") + ip + string("</td>");
		res += string("<td>") + port + string("</td>");
		res += string("<td>");
		if (cur_server[3] == string("alive")) {
			res += string("RUNNING</td>");
		} else {
			res += string("DOWN</td>");
		}
		res += string("<td><a href=\"/stopbackend?index=") + to_string(i)
				+ string(
						"\">Stop</a><span> <span><a href=\"/startbackend?index=")
				+ to_string(i) + string("\">Start</a></td>");
		res += string("</tr>");
	}
	return res;
}

/*
 * read all servers into the deque
 */
void read_servers_from_file(string filename) {
	ifstream file(filename);
	string str;
	int count = 1;

	while (getline(file, str)) {
		char address[64];
		int port = 0;
		// copy the address and port number
		char copy[256];
		strcpy(copy, str.c_str());
		char *token;
		token = strtok(copy, ":"); // get the address
		strcpy(address, token);
		token = strtok(NULL, ":"); // get the the port
		port = atoi(token);

		Server tmp(address, port);
		servers.push_back(tmp);
		count++;
	}
}

/*
 * check the states of the ith server
 */
void check_state(int index) {
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	// build destination address according to the file and push it to the vector
	struct sockaddr_in dest;
	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(servers.at(index).port);
	inet_pton(AF_INET, servers.at(index).address.c_str(), &(dest.sin_addr));

	// check connection
	int ret_val = connect(sock, (sockaddr*) &dest, sizeof(dest));
	if (ret_val == 0) {
		pthread_mutex_lock (&lock1);
		servers.at(index).running = true;
		pthread_mutex_unlock(&lock1);
	} else {
		pthread_mutex_lock (&lock1);
		servers.at(index).running = false;
		pthread_mutex_unlock(&lock1);
	}
	close(sock);
}

/*
 * read all emails into a html string
 */
string read_all_emails(vector<email> emails, int type) {
	string res;
	for (int i = 0; i < emails.size(); i++) {
		string sender, rcpt, title, time;
		parse_header(emails.at(i).header, sender, rcpt, title, time);
		res += string("<a href=\"");
		if (type == 0) {
			res += string("/showinbox?id=") + to_string(i) + string("\">");
		} else if (type == 1) {
			res += string("/showsent?id=") + to_string(i) + string("\">");
		} else {
			res += string("/showtrash?id=") + to_string(i) + string("\">");
		}
		if (!emails.at(i).opened) {
			res += string("<b>");
		}

		res += string("<li class=\"list-group-item\" style=\"color: black;\">");
		if (type == 0) {
			res += sender;
		} else if (type == 1) {
			res += rcpt;
		}

		res += string("<span style=\"float: right;\">");
		res += title;
		res += string("</span></li>");

		if (!emails.at(i).opened) {
			res += string("</b>");
		}
		res += string("</a>");
	}

	return res;
}

/*
 * parse email header to corresponding string
 */
void parse_header(string header, string& sender, string& rcpt, string& title, string& time) {
	size_t p1 = header.find("|");
	size_t p2 = header.find("|", p1 + 1);
	size_t p3 = header.find("|", p2 + 1);
	size_t p4 = header.find("|", p3 + 1);
	size_t p5 = header.find("|", p4 + 1);

	sender = header.substr(p2 + 1, p3 - p2 - 1);
	rcpt = header.substr(p3 + 1, p4 - p3 - 1);
	title = header.substr(p4 + 1, p5 - p4 - 1);
	time = header.substr(p5 + 1);
}

/*
 * add command in the email page
 */
string parse_command(int type, string hash, int id) {
	string res;
	if (type == 0) {
		res += string("<form action=\"/replyid=") + to_string(id);
		res += string("\" method=\"GET\" class=\"flex-item\">");
		res += string("<button type=\"submit\" class=\"btn btn-primary\">Reply</button>");
		res += string("</form>");
		res += string("<form action=\"/forwardid=") + to_string(id);
		res += string("\" method=\"GET\" class=\"flex-item\">");
		res += string("<button type=\"submit\" class=\"btn btn-primary\">Forward</button>");
		res += string("</form>");
		res += string("<form action=\"/removeinboxhash=") + hash;
		res += string("\" ");
		res += string("method=\"GET\" class=\"flex-item\">");
		res += string("<button type=\"submit\" class=\"btn btn-primary\">Delete</button>");
		res += string("</form>");
	} else {
		res += string("<form action=\"/removeoutboxhash=") + hash;
		res += string("\" ");
		res += string("method=\"GET\" class=\"flex-item\">");
		res += string("<button type=\"submit\" class=\"btn btn-primary\">Delete</button>");
		res += string("</form>");
	}
	return res;

}

/*
 * get mail id
 */
int parse_mailid(string path) {
	int id = 0;
	int index = 0;
	while (path[index] != '=') index++;
	index++;
	while (path[index] != '?' && index < path.size()) {
		id = id * 10 + path[index++] - '0';
	}

	return id;
}

string for_display(const string& origin){
	string copy = origin;
	string::size_type pos = 0;
	while ((pos = copy.find("\r\n", pos)) != string::npos)
	    copy.replace(pos, 2, "<br>");
	return copy;
}

