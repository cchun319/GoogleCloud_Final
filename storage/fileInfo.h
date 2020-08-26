#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string.h>
using namespace std;
class file_info {
public:
	bool is_file; // true if is a file, false if is a folder
	string name;
	string id;
	string size;
	string time;
	file_info(){}
	file_info(string name,string id,bool is_file,string time, string size){
		this->id = id;
		this->is_file = is_file;
		this->name = name;
		this->time = time;
		this->size = size;
	}

};

//string parse_class_to_string(file_info f);

//file_info* parse_string_to_class(string str);
