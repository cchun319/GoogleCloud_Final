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
#include "Storage.h"

using namespace std;

file_info* Storage::parse_string_to_class(string str) {
	file_info* res = new file_info();
	string type = str.substr(0, 1);
	string name_id = str.substr(2);
	int i = 0;
	int previous = 0;
	vector<string> result;
	for (; i < name_id.length() - 1; i++) {
		if (name_id[i] == '|' && name_id[i + 1] == '|') {
			string temp = name_id.substr(previous, i - previous);
			result.push_back(temp);
			previous = i + 2;
		}
	}
	string last = name_id.substr(previous);
	result.push_back(last);
	res->id = result[0];
	res->name = result[1];
	res->time = result[2];
	res->size = result[3];
	if (type == "f") {
		res->is_file = true;
	} else {
		res->is_file = false;
	}
	return res;
}

void Storage::delete_recover( string& response, string index) {
	string get_data;
	send_get_command(index, get_data);
	if (get_data != "-ERR NULL") { //delete unfinished
		send_delete_command(index,response);
	} else {
		response = "+OK";
	}
}

//void Storage::cput_recover( string& response, string index,
//		bool is_add, string data, string filename) {
//	if (is_add) {
//		string get_response;
//		send_get_command(index, get_response);
//		if (get_response != "-ERR NULL") {
//			send_cput_command_add(index, get_response, data,  response,
//					filename);
//		}
//	} else {
//		string get_response;
//		send_get_command(index,get_response);
//		if (get_response != "-ERR NULL" && get_response.length() > 0) {
//			send_cput_command_delete(index, get_response, filename,
//					response);
//		}
//
//	}
//}


void Storage::server_recover() {
	masterConnect* m = new masterConnect();
	char message[100];
	memset(message,0,100);
	strcat(message, "Where <");
	strcat(message, this->userName.c_str());
	strcat(message, ">\r\n");
	string* newPrimaryAddr = m->sendto_backend(m->master_address, message);
	memset(message, 0, 100);
	string addr = *newPrimaryAddr;
	cout<<"receiving the new address: "<<addr<<endl;
	int i = 0;
	for (; i < addr.length(); i++) {
		if (addr[i] == ':')
			break;
	}

	string newIP = addr.substr(0, i);
	cout<<"new IP:"<<newIP<<endl;
	int newPort = atoi(addr.substr(i + 1).c_str());
	cout<<"newPort:"<<newPort<<endl;
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_pton(AF_INET, newIP.c_str(), &servaddr.sin_addr);
	servaddr.sin_port = htons(newPort);
    close(this->sockfd);
    this->sockfd = socket(PF_INET, SOCK_STREAM, 0);
	while(connect(this->sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1){
       this->sockfd = socket(PF_INET, SOCK_STREAM, 0);
	}
	cout<<"connection test: "<<endl;
	this->crash_flag = 0;
}

/*return row and column name*/
string Storage::command_parser(string userName, string column_name) {
	string result = "<" + userName + "> " + "<" + column_name + ">\r\n";
	return result;
}

/*check whether reading data from primary server is complete*/
bool Storage::complete_data(string str) {
	if (strstr(str.c_str(), "\r\n.\r\n") != NULL) {
		return true;
	} else {
		return false;
	}
}

/*check whether reply from primary server is complete*/
bool Storage::complete_reply(string str) {
	if (strstr(str.c_str(), "\r\n") != NULL) {
		return true;
	} else {
		return false;
	}
}

/*read data from backend after get command*/
string Storage::read_get_data() {
	string complete1 = "";
	while (1) {
		long int len = 7000000;
//		char buf[7000000];
		char* buf = (char*)malloc(len);
		memset(buf, 0, len);
		int count = read(this->sockfd, buf, len);
		string reply(buf);
		complete1.append(reply);
		free(buf);
		//primary server crashes
		if (strstr(complete1.c_str(),"-ERR Server shutting down") != NULL || count == 0) {
			this->crash_flag = 1;
			cout<<"-----server crashes,reconnecting-----"<<endl;
			return "SERVER CRASH";
		}
		//row column exist
		if (strstr(complete1.c_str(), "DATA\r\n") != NULL) {
			if (complete_data(complete1)) {
				if (complete1.length() <= 11) {
					return "";
				} else {
					string complete = complete1.substr(6,
							complete1.length() - 11);
					return complete;
				}
			}
		}
		//row column do not exist
		if (strstr(complete1.c_str(), "-ERR NULL") != NULL) {
			if (complete_reply(complete1)) {
				cout << complete1 << endl;
				return "-ERR NULL";
			}
		}
	}
}

/*read data from backend after put command*/
string Storage::read_put_reply() {
	string complete = "";
	while (1) {
		char buf[100];
		memset(buf, 0, 100);
		int i = read(this->sockfd, buf, 100);
		if (strstr(complete.c_str(),"-ERR Server shutting down") != NULL || i == 0) {
			this->crash_flag = 1;
			cout<<"-----server crashes-----"<<endl;
			return "SERVER CRASH";
		}
		string put_reply(buf);
		memset(buf, 0, 100);
		complete.append(put_reply);
		if (strstr(complete.c_str(), "\r\n") != NULL) {
			return complete;
		}
	}
}

void Storage::send_get_command(string index,string& response) {
	while (1) {
		string msg = "GET " + index;
		int re = write(this->sockfd, msg.c_str(), msg.length());
		response = read_get_data();
		if (response != "SERVER CRASH") {
			break;
		} else {
			server_recover();
			response.clear();
			send_get_command(index, response);
		}
	}
}

void Storage::send_put_command(string index, string data,
		string& response) {
	while (1) {
		string msg1 = "PUT " + index;
		write(this->sockfd, msg1.c_str(), msg1.length());
		string msg2 = "DATA\r\n" + data + "\r\n.\r\n";
		write(this->sockfd, msg2.c_str(), msg2.length());
		response = read_put_reply();
		if (response != "SERVER CRASH") {
			break;
		} else {
			server_recover();
			response.clear();
			send_put_command(index, data,response);
		}
	}
}

void Storage::send_cput_command(string index, string old_data, string new_data,
		 string& response) {
	while (1) {
		string msg1 = "CPUT " + index;
		string msg2 = "OLD\r\n" + old_data + "\r\n.\r\n";
		string msg3 = "NEW\r\n" + new_data + "\r\n.\r\n";
		write(this->sockfd, msg1.c_str(), msg1.length());
		write(this->sockfd, msg2.c_str(), msg2.length());
		write(this->sockfd, msg3.c_str(), msg3.length());
		response = read_put_reply();
		if (response != "SERVER CRASH") {
			break;
		} else {
			server_recover();
			response.clear();
			send_cput_command(index, old_data, new_data,response);
		}
	}
}

/*add file information in its path*/
void Storage::send_cput_command_add(string index, string old_data, string data,
		 string& response, string filename) {
	if (old_data.length() > 0) {
		vector<string> names = parse_content_fileNames(old_data);
		bool test_exist = exist_same_fileName(this->userName, filename, names);
		if (test_exist) {
			cout << "file with same name already exist" << endl;
			response = "-ERR";
			return;
		}
	}
	send_cput_command(index, old_data, old_data + data,response);
	while (strstr(response.c_str(), "-ERR") != NULL) {
		string get_response;
		send_get_command(index, get_response);
		if (get_response == "-ERR NULL") { //folder already been deleted
			break;
		} else {
			response.clear();
			if (get_response.length() > 0) {
				//test whether file with same name has been uploaded
				vector<string> names = parse_content_fileNames(get_response);
				bool test_exist = exist_same_fileName(this->userName, filename,
						names);
				if (test_exist) {
					cout << "file with same name already exist" << endl;
					break;
				}
			}
			send_cput_command(index, get_response, get_response + data,
					response);
			get_response.clear();
		}
	}
}

/*delete the file information in its path*/
bool Storage::send_cput_command_delete(string index, string old_data,
		string delete_name,  string& response) {
	bool has_deleted = true;
	vector<string> names = parse_content_fileNames(old_data);
	stringstream new_content;
	for (int i = 0; i < names.size(); i++) {
		file_info* f = parse_string_to_class(names[i]);
		//test whether file already deleted
		if (f->id != delete_name) {
			new_content << names[i] + "\n";
		} else {
			has_deleted = false;
		}
		delete f;
	}
	send_cput_command(index, old_data, new_content.str(), response);
	while (strstr(response.c_str(), "-ERR") != NULL) {
		string get_response;
		send_get_command(index, get_response);
		if (get_response == "-ERR NULL" || get_response.length() == 0) {
			break;
		} else {
			stringstream new_content1;
			vector<string> name1 = parse_content_fileNames(get_response);
			for (int i = 0; i < name1.size(); i++) {
				file_info* f = parse_string_to_class(name1[i]);
				if (f->id != delete_name) {
					new_content1 << name1[i] + "\n";
				} else {
					has_deleted = false;
				}
				delete f;
			}
			response.clear();
			send_cput_command(index, get_response, new_content1.str(),
					response);
			get_response.clear();
		}
	}
	return has_deleted;
}

void Storage::send_delete_command(string index,  string& response) {
	while (1) {
		string msg = "DELETE " + index;
		write(this->sockfd, msg.c_str(), msg.length());
		response = read_put_reply();
		if (response != "SERVER CRASH") {
			break;
		} else {
			server_recover();
			response.clear();
			send_delete_command(index, response);
		}
	}
}

//for GET command get the response from kvstore and parse it to seperated file names
vector<string> Storage::parse_content_fileNames(string str) {
	//get rid of DATACRLF and CRLF.CRLF 
	vector<string> names;
	if (str.length() == 0) {
		return names;
	}
	int i = 0;
	int previous = 0;
	for (; i < str.length(); i++) {
		if (str[i] == '\n') {
			string s = str.substr(previous, i - previous);
			names.push_back(s);
			previous = i + 1;
		}
	}
	return names;
}

/*test whether file with same name already exist*/
bool Storage::exist_same_fileName(string userName, string filename,
		vector<string> exist_names) {
	if (exist_names.size() > 0) {
		for (int i = 0; i < exist_names.size(); i++) {
			file_info* f = parse_string_to_class(exist_names[i]);
			if (f->name == filename) {

				delete f;
				return true;
			}
			delete f;
		}
	}
	return false;
}

// the time is in millionsecond
//using the first time the file is uploaded as unique ID
string Storage::getTime_as_ID() {
	char timeBuf[256];
	struct timeval tv;
	struct timezone tz;
	struct tm *tm;
	gettimeofday(&tv, &tz);
	tm = localtime(&tv.tv_sec);

	sprintf(timeBuf, "%04d:%02d:%02d:%02d:%02d:%02d.%06ld", tm->tm_year + 1900,
			tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
			(tv.tv_usec / 1000));
	return timeBuf;
}

string Storage::create_time() {
	char timeBuf[256];
	struct timeval tv;
	struct timezone tz;
	struct tm *tm;
	gettimeofday(&tv, &tz);
	tm = localtime(&tv.tv_sec);
	sprintf(timeBuf, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900,
			tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	return timeBuf;
}

/*register part*/
bool Storage::resgister(string userName, string password) {
	string index = command_parser(userName, "password");
	string data;
	send_get_command(index, data);
	cout << "get <userName> <password>" << endl;
	if (data == "-ERR NULL") {
		string temp;
		send_cput_command(index,"NULL",password,temp);
		if(strstr(temp.c_str(),"-ERR") != NULL) {
			cout<<"user already exist,unable to register"<<endl;
			return false;
		}
		//send_put_command(index, password, temp);
		cout << "register successfully" << endl;
		string storage_index = command_parser(userName, "storage:name");
		string temp2;
		send_put_command(storage_index, "storage",temp2);
		string storage_index2 = command_parser(userName, "storage:content");
		string temp3;
		send_put_command(storage_index2, "", temp3);

		string data_test;
		send_get_command(storage_index, data_test);
		open_folder(this->userName, "storage");

		return true;
	} else {
		cout << "user already exist,unable to register" << endl;

		return false;
	}

}

/*sign in part*/
bool Storage::sign_in(string userName, string password) {
	string result = "<" + userName + "> " + "<" + "password" + ">\r\n";
	string data;
	send_get_command(result, data);
	if (data == password) {
		cout << "sign in successfully" << endl;
		open_folder(this->userName, "storage");

		return true;
	}

	return false;
}

/*upload file in a certain path*/
//the content is  f/o:ID||name||time||size
bool Storage::upload_file(string userName, string pathID, string filename,
		vector<char>& content,  string size) {
	string index = command_parser(userName, pathID + ":content");
	string complete;
	send_get_command(index, complete);

	if (complete == "-ERR NULL") {

		return false;
	}
	string fileID = getTime_as_ID();
	string time = create_time();
	string total_fileID = "f:" + fileID + "||" + filename + "||" + time + "||"
			+ size;
	vector<string> names = parse_content_fileNames(complete);
	string cput_reply;
	cout<<"uploading,start put command"<<endl;
	if (names.size() == 0) {
		string new_complete = total_fileID + "\n";
		send_cput_command_add(index, "", new_complete,  cput_reply,
				filename);
	} else {
		bool exist = exist_same_fileName(userName, filename, names);
		if (exist) {
			cout << "a file with the same name already exist" << endl;

			return false;
		} else {
			string new_complete = complete + total_fileID + "\n";
			send_cput_command_add(index, complete, total_fileID + "\n",
					cput_reply, filename);
		}
	}
	cout << "cput reply: " << cput_reply << endl;
	if (strstr(cput_reply.c_str(), "+OK") != NULL) {
		string index_name = command_parser(userName, fileID + ":name");
		string index_content = command_parser(userName, fileID + ":content");
		fileHandler* fh = new fileHandler();
		string str = fh->convert_binary_to_str(content);
		cout<<"uploading,start put command"<<endl;
		string put_reply;
		send_put_command(index_name, filename,  put_reply);
		string put_reply2;
		send_put_command(index_content, str,  put_reply2);
		file_info* f = new file_info(filename, fileID, true, time, size);
		this->files_in_path.insert(make_pair(filename, f));
		cout << "upload " + filename + " successfully" << endl;

		return true;
	} else {

		return false;
	}
}

string Storage::nameToID(string name) {
	return this->files_in_path[name]->id;
}

/*return null if the folder is unable to open*/
vector<file_info*>* Storage::open_folder(string userName, string foldername) {

	string folderID = nameToID(foldername);
	vector<file_info*>* files = new vector<file_info*>;
	string index = command_parser(userName, folderID + ":content");
	string data;
	send_get_command(index,data);
	cout << "data:" << data << endl;
	if (data == "-ERR NULL") {
		return NULL;
	}
	if (data.length() == 0) {
		file_info* f = new file_info(foldername, folderID, false,
				this->files_in_path[foldername]->time,
				this->files_in_path[foldername]->size);
		this->path.push_back(f);

		this->files_in_path.clear();
		cout << "open folder successfully" << endl;
		return files;
	} else {
		file_info* f = new file_info(foldername, folderID, false,
				this->files_in_path[foldername]->time,
				this->files_in_path[foldername]->size);
		this->path.push_back(f);
		this->files_in_path.clear();
		vector<string> names = parse_content_fileNames(data);
		for (int i = 0; i < names.size(); i++) {
			cout << names[i] << endl;
			file_info* f = parse_string_to_class(names[i]);
			(files)->push_back(f);
			this->files_in_path.insert(make_pair(f->name, f));
		}

		return files;
	}
}

vector<file_info*>* Storage::refresh_upload() {
	string folderID = this->path.at(this->path.size() - 1)->id;
	string foldername = this->path.at(this->path.size() - 1)->name;
	string userName = this->userName;
	vector<file_info*>* files = new vector<file_info*>;
	string index = command_parser(userName, folderID + ":content");
	string data;
	send_get_command(index, data);
	if (data == "-ERR NULL")
		return NULL;
	if (data.length() == 0) {
		this->files_in_path.clear();
	} else {
		this->files_in_path.clear();
		vector<string> names = parse_content_fileNames(data);
		for (int i = 0; i < names.size(); i++) {
			file_info* f = parse_string_to_class(names[i]);
			(*files).push_back(f);
			this->files_in_path.insert(make_pair(f->name, f));
		}

	}
	return files;
}

bool Storage::create_folder(string userName, string pathID, string folderName) {
	string index = command_parser(userName, pathID + ":content");
	string data;
	send_get_command(index, data);
	if (data == "-ERR NULL")
		return false;
	cout << "create folder, old data is: " << data << endl;
	string folderID = getTime_as_ID();
	string time = create_time();
	string full_name = "o:" + folderID + "||" + folderName + "||" + time
			+ "||0";

	cout << "the folder ID is:" << full_name << endl;
	if (data.length() > 0) {
		vector<string> names = parse_content_fileNames(data);
		if (exist_same_fileName(userName, folderName, names)) {
			cout << "exist file with same name:" + folderName << endl;
			return false;
		}
	}
	string new_data = data + full_name + "\n";
	string cput_reply;
	send_cput_command_add(index, data, full_name + "\n",  cput_reply,
			folderName);

	if (strstr(cput_reply.c_str(), "+OK") != NULL) {
		string put_index1 = command_parser(userName, folderID + ":name");
		string put_index2 = command_parser(userName, folderID + ":content");
		string put_reply1;
		send_put_command(put_index1, folderName,  put_reply1);
		string put_reply2;
		send_put_command(put_index2, "",  put_reply2);
		file_info* f = new file_info(folderName, folderID, false, time, "0");
		this->files_in_path.insert(make_pair(folderName, f));
		refresh_upload();
		return true;
	} else {
		cout << "create folder fails" << endl;
		return false;
	}
}

bool Storage::download_file(string userName, string filename,
		vector<char>& content) {
	cout<<"downloadng the file name is:"<<filename<<endl;
	string fileID = this->files_in_path[filename]->id;
	cout<<"fileIDD is:::"<<fileID<<endl;
	string index = command_parser(userName, fileID + ":content");
	string data;
	send_get_command(index, data);
	if (data == "-ERR NULL") {
		cout << "download fails,the file has been deleted by someone else"
				<< endl;
		return false;
	}
	fileHandler* fh = new fileHandler();

	content = fh->convert_str_to_binary(data);
	delete fh;
	cout << "download successfully" << endl;
	return true;
}

/*delete one file/folder in current path*/
bool Storage::delete_file(string userName, string pathID, string filename) {
	string fileID = nameToID(filename);
	string index = command_parser(userName, pathID + ":content");
	string data;
	send_get_command(index, data);
	if (data == "-ERR NULL" || data.length() == 0) {
		cout << "delete file fails, no such file" << endl;
		return false;
	}
	string cput_reply;
	bool try_delete = send_cput_command_delete(index, data, fileID,
			cput_reply);
	if (try_delete) {
		cout << "no such file/folder,already deleted" << endl;
		return false;
	}
	if (strncasecmp(cput_reply.c_str(), "+OK", 3) == 0) {
		string delete_index1 = command_parser(userName, fileID + ":name");
		string delete_reply1;
		send_delete_command(delete_index1,  delete_reply1);
		string delete_index2 = command_parser(userName, fileID + ":content");
		string delete_reply2;
		send_delete_command(delete_index2,  delete_reply2);
		delete (this->files_in_path[filename]);
		this->files_in_path.erase(filename);
		cout << "delete file/folder:" + filename + " successfully" << endl;
		return true;
	} else {
		cout << "delete file fails" << endl;
		return false;
	}
}

/*rename file or folder*/
bool Storage::rename_file(string userName, string old_name, string new_name) {
	if(this->files_in_path.count(old_name) == 0){
		return false;
	}
	string pathID = this->path.at(this->path.size() -1)->id;
	string fileID = this->files_in_path[old_name]->id;
	 bool is_file = this->files_in_path[old_name]->is_file;
	string path_index = command_parser(userName, pathID + ":content");
	string path_content;
	send_get_command(path_index,path_content);
	if (path_content == "-ERR NULL" || path_content.length() == 0)
		return false;
	vector<string> names = parse_content_fileNames(path_content);
	string new_contents = "";
	for (int i = 0; i < names.size(); i++) {
		file_info* f = parse_string_to_class(names[i]);
		if (f->name != old_name) {
			new_contents.append(names[i] + "\n");
		} else {
			string new_content;
			if(is_file){
			new_content = "f:" + f->id + "||" + new_name + "||" + f->time
					+ "||" + f->size + "\n";
			}else {
				new_content = "o:" + f->id + "||" + new_name + "||" + f->time
									+ "||" + f->size + "\n";
			}
			new_contents.append(new_content);
		}
		delete f;
	}
	string index = command_parser(userName, fileID + ":name");
	string cput_reply;
	send_cput_command(path_index, path_content, new_contents,
			cput_reply);
	while (strstr(cput_reply.c_str(), "-ERR") != NULL) {
		path_content.clear();
		send_get_command(path_index, path_content);
		if (path_content == "-ERR NULL" || path_content.length() == 0) {
			return false;
		} else {
			cput_reply.clear();
			names.clear();
			names = parse_content_fileNames(path_content);
			string new_contents = "";
			for (int i = 0; i < names.size(); i++) {
				file_info* f = parse_string_to_class(names[i]);
				if (f->name != old_name) {
					new_contents.append(names[i] + "\n");
				} else {
					string new_content;
					if(is_file){
					new_content = "f:" + f->id + "||" + new_name + "||"
							+ f->time + "||" + f->size + "\n";
					}else {
						new_content = "o:" + f->id + "||" + new_name + "||"
													+ f->time + "||" + f->size + "\n";
					}
					new_contents.append(new_content);
				}
				delete f;
			}
			send_cput_command(path_index, path_content, new_contents,
					cput_reply);
		}
	}
	if (strncasecmp(cput_reply.c_str(), "+OK", 3) == 0) {
		string put_response;
		send_put_command(index, new_name,  put_response);
		cout << "rename file: " << old_name << "to " << new_name
				<< "successfully" << endl;
		return true;
	} else {
		return false;
	}
}

//return only folders
/*the method is used for moving file/folder
 * when click move, all the accessible folder name will be shown*/
vector<file_info*>* Storage::get_accessible_movingPath(string filename) {
	string userName = this->userName;
	vector<file_info*>* res = new vector<file_info*>;
	unordered_map<string, file_info*>::iterator it;
	for (it = this->files_in_path.begin(); it != this->files_in_path.end();
			it++) {
		if ((!it->second->is_file) &&(it->second->name != filename)) {
			(*res).push_back(it->second);
		}
	}
	return res;
}



bool Storage::recover_old_path(string old_pathID, string filename,
		string fileID, bool is_file, string time, string size) {
	string get_reply;
	string old_path_index = command_parser(this->userName,
			old_pathID + ":content");
	send_get_command(old_path_index,get_reply);
	string total_content;
	if (is_file) {
		total_content = "f:" + fileID + "||" + filename + "||" + time + "||"
				+ size + "\n";
	} else {
		total_content = "o:" + fileID + "||" + filename + "||" + time + "||"
				+ size + "\n";
	}
	if (get_reply == "-ERR NULL") {
		cout << "old folder has been deleted" << endl;
		return false;
	} else {
		string cput_reply;
		//string new_content = get_reply + total_content + "\n";
		send_cput_command_add(old_path_index, get_reply, total_content,
				cput_reply, filename);
		if (strstr(cput_reply.c_str(), "-ERR") != NULL) {
			cout << "old folder has been deleted" << endl;
			return false;
		}
		return true;
	}
}

/*move file/folder to another folder which is in the same path*/
bool Storage::move_file(string new_pathname, string filename) {
	string old_pathID = this->path.at(this->path.size() - 1)->id;
	string new_pathID = this->files_in_path[new_pathname]->id;
	string fileID = this->files_in_path[filename]->id;
	bool is_file = this->files_in_path[filename]->is_file;
	string time = this->files_in_path[filename]->time;
	string size = this->files_in_path[filename]->size;
	string userName = this->userName;
	string old_path_index = command_parser(userName, old_pathID + ":content");
	string old_path_content;
	send_get_command(old_path_index,old_path_content);
	if (old_path_content == "-ERR NULL" || old_path_content.length() == 0) {

		return false;
	} else {
		string cput_reply;
		bool d = send_cput_command_delete(old_path_index, old_path_content,
				fileID, cput_reply);
		if (d) {
			cout << "the file has been deleted by someone else" << endl;

			return false;
		}
		if (strstr(cput_reply.c_str(), "+OK") != NULL) {
			//change new path
			string new_path_index = command_parser(userName,
					new_pathID + ":content");
			string new_path_content;
			send_get_command(new_path_index,new_path_content);
			if (strstr(new_path_content.c_str(), "-ERR NULL") != NULL) {
				//recover old path
				cout << "the folder moved to has been deleted" << endl;
				bool recover = recover_old_path(old_pathID, filename, fileID,
						is_file, time, size);

				return false;
			} else {
				string new_path_content_update;
				if (is_file) {
					new_path_content_update = "f:" + fileID + "||" + filename
							+ "||" + time + "||" + size + "\n";
				} else {
					new_path_content_update = "o:" + fileID + "||" + filename
							+ "||" + time + "||" + size + "\n";
				}
				string cput_reply1;
				send_cput_command_add(new_path_index, new_path_content,
						new_path_content_update,  cput_reply1, filename);
				if (strstr(cput_reply1.c_str(), "-ERR") != NULL) {
					//the new_folder has been deleted, recover old path
					bool recover = recover_old_path(old_pathID, filename,
							fileID, is_file, time, size);
					cout << "moving file fails" << endl;

					return false;
				} else {
					cout << "move file/folder " + filename + " successfully"
							<< endl;
					refresh_upload();

					return true;
				}
			}

		} else {
			cout << "uable to move file/folder, since someone else has deleted";

			return false;
		}
	}
}

/*delete folder the user current in*/
bool Storage::delete_folder() {
	string pathName = this->path.at(this->path.size() - 1)->name;
	string folderPathID = this->path.at(this->path.size() - 2)->id;
	goBack(this->path.size() - 2);
	return delete_file(this->userName, folderPathID, pathName);
}

vector<file_info*>* Storage::goBack(int index) {
	int size = this->path.size();
	for(int i = size - 1; i >= index+1; i--){
		delete this->path.at(i);
		this->path.erase(this->path.begin() + i);
	}
	return refresh_upload();
}

vector<file_info*>* Storage::reload_page(int index) {
	if (index < this->path.size() - 1) {
		return goBack(index);
	} else {
		return refresh_upload();
	}
}

