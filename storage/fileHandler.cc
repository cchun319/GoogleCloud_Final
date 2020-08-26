#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <bits/stdc++.h>
#include "fileHandler.h"
using namespace std;

/*convert char to hex and return a string for hexes*/
string fileHandler::convert_binary_to_str(vector<char>& content) {
	stringstream res;
	stringstream test;
	vector<char> result;
	cout<<"input vector  size is: "<<content.size()<<endl;
	for (int i = 0; i < content.size(); i ++) {
		char* temp = (char*)malloc(10);
		char temp1[3];
		memset(temp1,0,3);
		sprintf(temp,"%02x",(unsigned char)content[i]);
		result.push_back(temp[0]);
		result.push_back(temp[1]);
		temp1[0] = temp[0];
		temp1[1] = temp[1];
		temp1[2] = '\0';
		free(temp);
		res << temp1;
	}
	string result1(result.begin(),result.end());
	cout<<"string length1 is :" <<result1.length()<<endl;
	return result1;
}

/*convert hex string to char array*/
vector<char> fileHandler::convert_str_to_binary(string str) {
	cout<<"string length2 is :" <<str.length()<<endl;
	stringstream s(str);
	vector<char> result;
   stringstream test;
	for(int i = 0; i <str.length()-1; i = i+2 ){
	      stringstream temp(str.substr(i,2));
	      unsigned int c;
	      temp>>hex>>c;
	      result.push_back(static_cast<unsigned char> (c));
	      test<<(static_cast<unsigned char> (c));
	  }
	return result;
}
