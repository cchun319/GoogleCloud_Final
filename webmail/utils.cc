/*
 * utils.cc
 *
 *  Created on: Apr 2, 2018
 *      Author: cis505
 */

#include "utils.h"

const int BUFF_SIZE = 5000;

string sendto_backend(string& master_addr, const char* message){
	// init a sock address
	struct sockaddr_in master;
	bzero(&master, sizeof(master));
	master.sin_family = AF_INET;

	// parse address info
	size_t found = master_addr.find(':');
	string ip = master_addr.substr(0, found);
	string port = master_addr.substr(found+1);
	inet_pton(AF_INET, ip.c_str() , &master.sin_addr);
	master.sin_port = htons(atoi(port.c_str()));

	// open a socket
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		cerr << "cannot open socket\r\n";
	    exit(4);
	}
    // set port for reuse
	const int REUSE = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &REUSE, sizeof(REUSE));

	// connect to master
	connect(sock, (struct sockaddr*)&master, sizeof(master));

	// write message and read response
	do_write(sock, message, strlen(message));
    cout << "send: " << message;
	string response = do_read(sock);
    cout << "respond: "<<response << endl;

    // close session in primary node
    do_write(sock, "QUIT\r\n", 6);

    // close socket
	close(sock);

	return response;
}

void do_write(int fd, const char* buff, int len) {
	// keep writing until buff is sent
	int sent = 0;
	while (sent < len) {
		int n = write(fd, &buff[sent],len-sent);
		if (n<0){
			cerr << "cannot write buffer to socket\r\n";
			exit(5);
		}
		sent += n;
	}
}

string do_read(int fd){
	// reading according to different backend(master or primary) and difference response (NULL, OK, DATA)
	string response;

	char buff[BUFF_SIZE];
	memset(buff, 0, BUFF_SIZE); // necessary, otherwise previous call will affect
	char* curr = buff;
	bool QUIT = false; bool DATA = false;

	while(true){
		char* end;
		// expect to read (BUF_SIZE-curr_len) bytes to curr, assuming already read (curr_len) bytes
		int len = read(fd, curr, BUFF_SIZE-strlen(buff));

		while((end=strstr(buff,"\r\n"))!=NULL){ // check whether command terminate
			// strstr return 1st index, move to real end
			end += 2;

			if (DATA){ // reading continue
				if (strcmp(buff, ".\r\n")!=0){
					response += string(buff, end-buff);
				} else { // reading end
					QUIT = true;
				}
			} else {
				// handle command
				char command[6];
				strncpy(command, buff, 6);
				command[6] = '\0';
				if (strcasecmp(command, "DATA\r\n") == 0){
					DATA = true;
				} else if (strncasecmp(command, "-ERR", 4) == 0){
					response = string(buff, end-buff);
					QUIT = true;
				} else if (strncasecmp(command, "+OK", 3) == 0){
					response = string(buff, end-buff);
					QUIT = true;
				} else if (strcasecmp(command, "SERVER") == 0){ // from master
					response = string(buff, end-buff);
					QUIT = true;
				}
			}
			if (QUIT) break;
			clear_buffer(buff, end);
		}

		if (QUIT) break;
		// no full line in buffer now, move curr to end
		curr = buff;
		while (*curr != '\0') {
			curr++;
		}
	}
	return response;

}

void computeDigest(char *data, int dataLengthBytes, unsigned char *digestBuffer)
{
	/* The digest will be written to digestBuffer, which must be at least MD5_DIGEST_LENGTH bytes long */

	MD5_CTX c;
	MD5_Init(&c);
	MD5_Update(&c, data, dataLengthBytes);
	MD5_Final(digestBuffer, &c);
}

string hash_MD5(string& data){ // generate a hash based on email content
	unsigned char digestBuffer[MD5_DIGEST_LENGTH];
	char uid[MD5_DIGEST_LENGTH+1];

	char d[data.length()+1];
	strcpy(d, data.c_str());

	computeDigest(d, strlen(d), digestBuffer);

	for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
		sprintf(uid + i, "%02x", digestBuffer[i]);
	}
	return string(uid);
}

void clear_buffer(char *buff, char *end){
	char* curr = buff;
	// move remaining to the start
	while (*end != '\0') {
		*curr++ = *end;
		*end++ = '\0';
	}
	// clear rest of last command
	while (*curr != '\0') {
		*curr++ = '\0';
	}
}

string ask_master(string& master_address, string &user){
	string backend_message = "WHERE <" + user + ">\r\n";
	string response = sendto_backend(master_address, backend_message.c_str());
	string ans;
	if (response.compare("NULL")==0){
		ans = "NULL";
	} else {
		size_t f1 = response.find('<');
		size_t f2 = response.find('>');
		ans = response.substr(f1+1, f2-f1-1);
	}
	backend_message = "QUIT\r\n";
	response = sendto_backend(master_address, backend_message.c_str());
	return ans;
}

