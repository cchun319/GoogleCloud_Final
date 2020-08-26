#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string.h>

using namespace std;
string parse_class_to_string(file_info f) {
	string res;
	if(f.is_file){
		res = "f:"+f.id+"||"+f.name;
	} else {
        res = "o:"+f.id+"||"+f.name;
	}
	return res;
}

file_info* parse_string_to_class(string str){
	file_info* res = new file_info();
	string type = str.substr(0,1);
	string name_id = str.substr(2);
	int i = 0;
	for(; i < name_id.length()-1;i++){
		if(name_id[i] == '|' && name_id[i+1] == '|'){
			break;
		}
	}
	string id = name_id.substr(0,i);
	string name = name_id.substr(i+2);
	res->id = id;
	res->name = name;
    if(type == "f") {
    	res->is_file = true;
    } else {
    	res->is_file = false;
    }
    return res;
}
