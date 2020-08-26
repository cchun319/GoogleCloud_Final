/*
 * Author:Xiang Li
 *
 * the Storage API for penn drive
 * uolpad file
 * download file
 * create folder
 * delete file/folder
 * move file/folder
 * rename file/folder
 * */
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <ctime>
#include <bitset>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iomanip>
#include <unordered_map>
#include "fileHandler.h"
#include "fileInfo.h"
#include "masterConnect.h"
using namespace std;
class Storage {
public:
	vector<file_info*> path;
	unordered_map<string, file_info*> files_in_path;

	string master_address;
	string userName;
	int sockfd;
	int crash_flag = 0;

	Storage(string userName) {
		this->userName = userName;
		int sock_master = socket(PF_INET, SOCK_STREAM, 0);
		masterConnect* m = new masterConnect();
		char message[100];
		memset(message,0,100);
		strcat(message, "Where <");
		strcat(message, this->userName.c_str());
		strcat(message, ">\r\n");
		string* newPrimaryAddr = m->sendto_backend(m->master_address, message);
		memset(message, 0, 100);
		close(sock_master);
		string addr = *newPrimaryAddr;
		cout<<"primary address is: "<<addr<<endl;
		int i = 0;
		for (; i < addr.length(); i++) {
			if (addr[i] == ':')
				break;
		}
		string ipaddr_str = addr.substr(0, i);
		const char* ipaddr = ipaddr_str.c_str();
		int portNum = atoi(addr.substr(i + 1).c_str());

		sockfd = socket(PF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) {
			fprintf(stderr, "Cannot open socket (%s)\n", strerror(errno));
			exit(1);
		}
		struct sockaddr_in servaddr;
		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		inet_pton(AF_INET, ipaddr, &servaddr.sin_addr);
		servaddr.sin_port = htons(portNum);
		connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
		file_info* f = new file_info("storage", "storage", false, "0", "0");
		this->files_in_path.insert(make_pair("storage", f));
	}

	~Storage(){
		write(this->sockfd,"QUIT\r\n",6);
		close(this->sockfd);
	}

	file_info* parse_string_to_class(string str);
	void delete_recover( string& response, string index);
//	void cput_recover( string& response, string index, bool is_add,
//			string data, string filename);
	string command_parser(string userName, string column_name);
	void send_get_command(string index,string& response);
	void send_put_command(string index, string data,
			string& response);
	void send_cput_command(string index, string old_data, string new_data,
			 string& response);
	void send_delete_command(string index,  string& response);
	bool complete_data(string str);
	bool complete_reply(string str);
	string read_get_data();
	vector<string> parse_content_fileNames(string str);
	bool exist_same_fileName(string userName, string filename,
			vector<string> exist_names);
	string getTime_as_ID();
	string read_put_reply();
	bool resgister(string userName, string password);
	bool sign_in(string userName, string password);
	bool upload_file(string userName, string pathID, string filename,
			vector<char>& content,  string size);
	string nameToID(string name);
	vector<file_info*>* open_folder(string userName, string foldername);
	bool create_folder(string userName, string pathID, string folderName);
	bool download_file(string userName, string filename,
			vector<char>& content);
	bool delete_file(string userName, string pathID, string filename);
	bool rename_file(string userName,string old_name, string new_name);
	vector<file_info*>* get_accessible_movingPath(string filename);
	bool recover_old_path(string old_pathID, string filename, string fileID,
			bool is_file, string time, string size);
	bool move_file(string new_pathname, string filename);
	bool delete_folder();
	vector<file_info*>* refresh_upload();
	string create_time();
	void send_cput_command_add(string index, string old_data, string data,
			 string& response, string filename);
	bool send_cput_command_delete(string index, string old_data,
			string delete_name,  string& response);
	vector<file_info*>* goBack(int index);
	vector<file_info*>* reload_page(int index);
	void server_recover();
};

