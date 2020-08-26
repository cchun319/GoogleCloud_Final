#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sstream>
using namespace std;

class fileHandler{
public:
	string convert_binary_to_str(vector<char>& content);
	vector<char> convert_str_to_binary(string str);
};
