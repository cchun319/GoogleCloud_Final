#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <signal.h>
#include <vector>
#include <set>
#include <time.h>
#include <unordered_map>
#include "utils.h"
#include "deliver.h"
using namespace std;

// message
const char* SERVER_READY    = "220 localhost smtp server ready\r\n";
const char* SERVICE_CLOSE   = "221 localhost service closing transmission channel\r\n";
const char* SERVICE_NA      = "421 localhost service not available, closing transmission channel\r\n";
const char* HELO            = "250 localhost\r\n";
const char* OK              = "250 OK\r\n";
const char* START_MAIL      = "354 Start mail input; end with <CRLF>.\r\n";
const char* UNKNOWN_CMD     = "500 Syntax error, command unrecognized\r\n";
const char* SYNTAX_ERR      = "501 Syntax error in parameters or arguments\r\n";
const char* BAD_SEQ         = "503 Bad sequence of commands\r\n";
const char* MAILBOX_NA      = "550 Requested action not taken: mailbox unavailable\r\n";
const char* NEW_CONN        = "New connection\r\n";
const char* CLOSE_CONN 		= "Connection closed\r\n";

// global
const int BUFF_SIZE = 5000;
const int CMD_SIZE = 5;
const int RSP_SIZE = 100;
const int MAILBOX_SIZE = 50;
bool DEBUG = false;
vector<int> SOCKETS;
vector<pthread_t> THREADS;
string MASTER_ADDR;

// message queue
const char* MQUEUE_DIR = "./outbox/mqueue";
static pthread_mutex_t MUTEX; // mutex

int smtp_server(unsigned int port);
void signal_handler(int arg);
void *worker_thread(void *arg);
void handle_helo(int comm_fd, int* state, char* buff);
void handle_from(int comm_fd, int* state, char* buff, string& sender);
void handle_to(int comm_fd, int* state, char* buff, set<string>& rcpts);
void handle_data(int comm_fd, int* state, char* buff, char* end, string& data, string& sender, set<string>& rcpts);
void handle_rset(int comm_fd, int* state, string& data, string& sender, set<string>& rcpts);
void handle_response(int comm_fd, const char* response);
void parse_mailbox(char* dest, char* src);


int main(int argc, char *argv[]){
	int c;
	unsigned int port = 7000;

	// getopt() for command parsing
	while((c=getopt(argc,argv,"p:av"))!=-1){
		switch(c){
		case 'p': //set port num
			port = atoi(optarg);
			break;
		case 'a': //exit
	    	cerr << "Wudao Ling (wudao) @UPenn\r\n";
	    	exit(1);
		case 'v': //debug mode
			DEBUG = true;
			break;
		default:
			cerr <<"Syntax: "<< argv[0] << " [-p port] [-a] [-v] <master node address>\r\n";
			exit(1);
		}
	}

	// check master node address: remaining non-option arguments
	if (optind == argc) {
		cerr <<"Syntax: "<< argv[0] << " [-p port] [-a] [-v] <master node address>\r\n";
		exit(1);
	}

	MASTER_ADDR= string(argv[optind]);

    //smtp server
    smtp_server(port);
}

int smtp_server(unsigned int port){
	// handle ctrl+c signal
	signal(SIGINT, signal_handler);

	// init mutex
	pthread_mutex_init(&MUTEX, NULL);

    // create a new socket (TCP)
	int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		cerr << "cannot open socket\r\n";
	    exit(2);
	}
	SOCKETS.push_back(listen_fd);

    // set port for reuse
	const int REUSE = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &REUSE, sizeof(REUSE));

	// bind server with a port
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htons(INADDR_ANY);
	servaddr.sin_port = htons(port);
	bind(listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr));

	// listen and accept
	listen(listen_fd, 100); // length of queue of pending connections
	while(true){
		struct sockaddr_in clientaddr;
		socklen_t clientaddrlen = sizeof(clientaddr);
		int fd = accept(listen_fd, (struct sockaddr*)&clientaddr, &clientaddrlen);
		SOCKETS.push_back(fd);

		// dispatcher assign connection to worker threads, concurrent <=100
		pthread_t thread;
		pthread_create(&thread, NULL, worker_thread, &fd);
		THREADS.push_back(thread);
		// mark resource of thread for reclaim when it terminates
		pthread_detach(thread);
    }
    return 0;
}

void signal_handler(int arg) {
	// close listen_fd first to prevent incoming sockets
	close(SOCKETS[0]);

	for (int i = 1; i < SOCKETS.size(); i++) {
		write(SOCKETS[i], SERVICE_NA, strlen(SERVICE_NA));
		close(SOCKETS[i]);
		pthread_kill(THREADS[i - 1], 0);
	}
	exit(3);
}

void *worker_thread(void *arg){
	int comm_fd = *(int*)arg;

	// send greeting message
	write(comm_fd, SERVER_READY, strlen(SERVER_READY));
	if (DEBUG){
		cerr << "["<< comm_fd << "] " << NEW_CONN;
	}

	int state = 0;
	// 0 - INIT
	// 1 - HELO/RSET
	// 2 - MAIL
	// 3 - RCPT
	// 4 - DATA start
	// 5 - DATA end
    // 6 - QUIT

	// maintain a buffer
	char buff[BUFF_SIZE];
	char* curr = buff;
	bool QUIT = false;

	// mail data
	string sender;
	set<string> rcpts;
	string data;

	while(true){
		char* end = new char;
		// expect to read (BUF_SIZE-curr_len) bytes to curr, assuming already read (curr_len) bytes
		int len = read(comm_fd, curr, BUFF_SIZE-strlen(buff));

		while((end=strstr(buff,"\r\n"))!=NULL){ // check whether command terminate
			// strstr return 1st index, move to real end
			end += 2;
			// read command
			char *command = new char[CMD_SIZE];
			strncpy(command, buff, CMD_SIZE);
			command[CMD_SIZE] = '\0'; // strncpy doesn't append \0 at the end

			if (DEBUG) {
				cerr << "["<< comm_fd << "] " << "C: "<< string(buff, end-buff);
			}

			// handle command
		    if (strcasecmp(command, "data\r") == 0 || state ==4){
		    	// DATA, which is followed by the text of the email and then a dot (.) on a line by itself
			    handle_data(comm_fd, &state, buff, end, data, sender, rcpts);
		    } else if (strcasecmp(command, "helo ") == 0){
		    	// HELO <domain>, which starts a connection
				handle_helo(comm_fd, &state, buff);
			} else if (strcasecmp(command, "mail ") == 0){
				// MAIL FROM:, which tells the server who the sender of the email is
				handle_from(comm_fd, &state, buff, sender);
			} else if (strcasecmp(command, "rcpt ") == 0){
				// RCPT TO:, which specifies the recipient
				handle_to(comm_fd, &state, buff, rcpts);
			} else if (strcasecmp(command, "rset\r") == 0){
				// RSET, aborts a mail transaction
				handle_rset(comm_fd, &state, data, sender, rcpts);
			} else if (strcasecmp(command, "noop\r") == 0){
				// NOOP, which does nothing
				handle_response(comm_fd, OK);
			} else if (strcasecmp(command, "quit\r") == 0 ) {
				// QUIT, which terminates the connection
				state = 6;
				QUIT = true;
				handle_response(comm_fd, SERVICE_CLOSE);
			} else { // unknown command
				handle_response(comm_fd, UNKNOWN_CMD);
			}

			delete[] command;
			if (QUIT) break;
			clear_buffer(buff, end);
		}

		if (QUIT) break;
		// free end after break, because end is NULL pointer here
		delete end;
		// no full line in buffer now, move curr to end
		curr = buff;
		while (*curr != '\0') {
			curr++;
		}
	}

    // terminate socket
	close(comm_fd);
	if (DEBUG) {
		cerr << "[" << comm_fd << "] " << CLOSE_CONN;
	}
	pthread_exit(NULL);
}

void handle_helo(int comm_fd, int* state, char* buff){
	// further check <domain>
	if (strlen(buff) <= CMD_SIZE){
		handle_response(comm_fd, SYNTAX_ERR);
	}

	if (*state > 1){ // already connect
		handle_response(comm_fd, BAD_SEQ);
	} else {
		*state = 1;
		handle_response(comm_fd, HELO);
	}
}

void handle_from(int comm_fd, int* state, char* buff, string& sender) {
	// further check command
	char extra[5];
	strncpy(extra, buff+CMD_SIZE, 5);
	extra[5] = '\0';
	if (strcasecmp(extra, "from:") != 0){
		handle_response(comm_fd, UNKNOWN_CMD);
	}

	if (*state != 1) {
		handle_response(comm_fd, BAD_SEQ);
	} else {
		char send[MAILBOX_SIZE];
		parse_mailbox(send, buff);

		string domain = string(send);
		size_t found = domain.find_last_of("@");

		if (domain.compare(found, 10, "@localhost") != 0 ){
			// only accept from localhost
			handle_response(comm_fd, SYNTAX_ERR);
		} else {
			*state = 2;
			sender = domain;
			handle_response(comm_fd, OK);
		}
	}
}

void handle_to(int comm_fd, int* state, char* buff, set<string>& rcpts) {
	// further check command
	char extra[3];
	strncpy(extra, buff+CMD_SIZE, 3);
	extra[3] = '\0';
	if (strcasecmp(extra, "to:") != 0){
		handle_response(comm_fd, UNKNOWN_CMD);
	}

	if (*state < 2 || *state > 3) {
		handle_response(comm_fd, BAD_SEQ);
	} else {
		char rcpt[MAILBOX_SIZE];
		parse_mailbox(rcpt, buff);

		string mailbox(rcpt);
		*state = 3;
		rcpts.insert(mailbox);
		handle_response(comm_fd, OK);
	}
}

void handle_data(int comm_fd, int* state, char* buff, char* end, string& data, string& sender, set<string>& rcpts){
	if (*state < 3 || *state > 4){
		handle_response(comm_fd, BAD_SEQ);
	} else if(*state ==3){
		*state = 4;
		handle_response(comm_fd, START_MAIL);
	} else if(strcmp(buff,".\r\n")!=0){ // data continue
		string line(buff,end-buff);
		data += line;
	} else { // data ends
		*state = 5;

		// prepare mail time
		time_t now = time(0);
		string time = ctime(&now); // convert raw time to calendar time
		time.pop_back(); time += "\r\n";

		// prepare mail title
		size_t f1 = data.find("Subject: ");
		if (f1==string::npos) { // can not find Subject
			handle_response(comm_fd, "email content must include Subject: \r\n");
		}
		size_t f2 = data.find("\r\n", f1+1);
		string title = data.substr(f1+9, f2-f1-9);

		// prepare node address for sender
		size_t found = sender.find_first_of("@");
		string sender_name = sender.substr(0,found);
		string sender_node = ask_master(MASTER_ADDR, sender_name);

		string hash = hash_MD5(data); // generate a unique column key by MD5 hashing
		string backend_message;
		string response;
		string value;

		// append mail in outbox key-value pair, hash first
	    while (true){
		    backend_message = "GET <" + sender_name + "> <mail/outbox/list>\r\n";
		    response = sendto_backend(sender_node, backend_message.c_str());

		    // init hash list, note this may be dangerous
		    if(response.compare(1,3, "ERR")==0) {
		    	backend_message = "PUT <" + sender_name + "> <mail/outbox/list>\r\nDATA\r\nlist\r\n.\r\n";
		    	response = sendto_backend(sender_node, backend_message.c_str());
		    	continue;
		    }

		    value = response;
		    size_t f1 = value.find(hash);
		    if(f1!=string::npos){ // remove dulplicate hash before
		    	size_t f2 = value.find("\r\n", f1+1);
		    	value.erase(f1, f2-f1+2);
		    }

		    value = hash + "\r\n" + value; // append at top, sorted by time
		    backend_message = "CPUT <" + sender_name + "> <mail/outbox/list>\r\nOLD\r\n" + response + ".\r\nNEW\r\n" + value +".\r\n";
	    	response = sendto_backend(sender_node, backend_message.c_str());
	    	if (response.compare(1,2, "OK")==0) break;
	    }

	    // prepare header for outbox
	    string header = "Header |localhost|" + sender + "|";
	    for (auto rcpt: rcpts) header += rcpt+",";
	    header.pop_back();
	    header += "|" + title + "|" + time;

		backend_message = "PUT <" + sender_name + "> <mail/outbox/" + hash + ">\r\nDATA\r\n" + header + data + ".\r\n";
		response = sendto_backend(sender_node, backend_message.c_str());

		// append mail in key-value pair, inbox
		for (auto rcpt: rcpts){
			size_t found = rcpt.find_first_of("@");
			string user_name = rcpt.substr(0, found);
			string server_name = rcpt.substr(found+1);
			string header = "Header |" + server_name + "|"+ sender + "|" + rcpt +"|" + title + "|" + time;

			if (server_name.compare(0,9, "localhost")==0){

				string node_addr = ask_master(MASTER_ADDR, user_name);
				if (node_addr.compare("NULL")==0) {
					handle_response(comm_fd, MAILBOX_NA);
					continue;
				}

				// append hash in inbox list
			    while (true){
				    backend_message = "GET <" + user_name + "> <mail/inbox/list>\r\n";
				    response = sendto_backend(node_addr, backend_message.c_str());

				    // init hash list, note this may be dangerous
				    if(response.compare(1,3, "ERR")==0) {
				    	backend_message = "PUT <" + user_name + "> <mail/inbox/list>\r\nDATA\r\nlist\r\n.\r\n";
				    	response = sendto_backend(node_addr, backend_message.c_str());
				    	continue;
				    }

				    value = response;
				    size_t f1 = value.find(hash);
				    if(f1!=string::npos){ // remove dulplicate hash before
				    	size_t f2 = value.find("\r\n", f1+1);
				    	value.erase(f1, f2-f1+2);
				    }

				    value = hash + "\r\n" + value; // append at top, sorted by time
				    backend_message = "CPUT <" + user_name + "> <mail/inbox/list>\r\nOLD\r\n" + response + ".\r\nNEW\r\n" + value +".\r\n";
			    	response = sendto_backend(node_addr, backend_message.c_str());
			    	if (response.compare(1,2, "OK")==0) break;
			    }

			    // put in mailbox
				backend_message = "PUT <" + user_name + "> <mail/inbox/" + hash + ">\r\nDATA\r\n" + header + data + ".\r\n";
				response = sendto_backend(node_addr, backend_message.c_str());

//     			// save into mailbox for debug purpose
//				ofstream mailbox;
//				mailbox.open("./inbox/" + user_name + "/" + hash, ios_base::trunc);
//				mailbox << header << data;
//				mailbox.close();
			} else {
				string from = "From: " + sender_name + " <" + sender + ">\r\n";
				string to = "To: " + user_name + " <" + rcpt + ">\r\n";
				string date = "Date: " + time;

				// save to local message queue, let deliver to forward
				pthread_mutex_lock(&MUTEX);
				ofstream mqueue;
				mqueue.open(MQUEUE_DIR, ios_base::app);
				mqueue << header << from << to << date << data;
				mqueue.close();
				pthread_mutex_unlock(&MUTEX);
			}

        // call deliver to mail relay
		mail_relay(MQUEUE_DIR);
		}

		// clear all
		data.clear();
		sender.clear();
		rcpts.clear();

		handle_response(comm_fd, OK);
	}

}

void handle_rset(int comm_fd, int* state, string& data, string& sender, set<string>& rcpts) {
	if (*state == 0) {
		handle_response(comm_fd, BAD_SEQ);
	} else {
		*state = 1;
		// clear all
		data.clear();
		sender.clear();
		rcpts.clear();

		handle_response(comm_fd, OK);
	}
}

void handle_response(int comm_fd, const char* response){
	write(comm_fd, response, strlen(response));
	if (DEBUG) {
		cerr << "["<< comm_fd << "] " << "S: "<< response;
	}
}

void parse_mailbox(char* dest, char* src){
	int i = 0, j = 0;
	while(src[i] != '<'){
		i++;
	}
	i++; // <> not included
	while(src[i] != '>'){
		dest[j++] = src[i++];
	}
	dest[j] = '\0';
}
