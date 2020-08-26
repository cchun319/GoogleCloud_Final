/*
 * deliver.cc
 *
 *  Created on: Apr 13, 2018
 *      Author: cis505
 */

#include "deliver.h"

// global
const int BUFF_SIZE = 2000;

int mail_relay(const char* DIR){
	vector<Message> messages;
	vector<string> headers;
	vector<string> header_info;

	string dir = string(DIR);
	read_mqueue(dir, messages, headers);

	for (int i = 0; i<headers.size(); i++){
		parse_header(headers[i], header_info);
		handle_forward(header_info[0], header_info[1], header_info[2], messages[i]);
	}
	update_mqueue(dir, messages, headers);

	return 0;
}

void read_mqueue(string& dir, vector<Message>& messages, vector<string>& headers){
	ifstream mqueue;
	mqueue.open(dir, ios_base::in);

	// read mailbox line by line until reading header, then save a message
	string data, line;
	string header = "Header ";
	while (getline(mqueue, line)) {
		if (line.compare(0, header.size(), header) == 0) {
			// end with last message
			messages.push_back(Message(data));
			data = "";

			// replace \n to \r\n and save line to headers
			line += "\r\n";
			headers.push_back(line);
		} else {
			// replace \n to \r\n and add line to data
			line += "\r\n";
			data += line;
		}
	}
	mqueue.close();

	// drop first message(empty), and add last message, except no messages
	if (!messages.empty()) {
		messages.erase(messages.begin());
		messages.push_back(Message(data));
	}
}

void update_mqueue(string& dir, vector<Message>& messages, vector<string>& headers){
	ofstream mqueue;
	mqueue.open(dir, ios_base::out | ios_base::trunc);

	for (int i=0; i<messages.size();i++){
		if (messages[i].deleted) continue; // drop deleted message
		mqueue << headers[i];
		mqueue << messages[i].data;
	}
	mqueue.close();
}

void parse_header(string& header, vector<string>& info){
	info.clear();
	size_t f1 = header.find('|');
	size_t f2 = header.find('|', f1+1);
	size_t f3 = header.find('|', f2+1);
	size_t f4 = header.find('|', f3+1);

	info.push_back(header.substr(f1+1, f2-f1-1)); // host domain
	info.push_back(header.substr(f2+1, f3-f2-1)); // sender
	info.push_back(header.substr(f3+1, f4-f3-1)); // recipient
}

void handle_forward(string& host, string& sender, string& rcpt, Message& message){
	// res_query reference: http://www.sourcexr.com/articles/2013/10/12/dns-records-query-with-res_query

	char address[BUFF_SIZE];
	unsigned char host_buffer[BUFF_SIZE];
	char buffer[BUFF_SIZE];

	int r = res_query(host.c_str(), C_IN, T_MX, host_buffer, BUFF_SIZE);

	ns_msg m; ns_rr rr;
	ns_initparse (host_buffer, r, &m);
	r = ns_msg_count(m, ns_s_an);
	int priority = INT_MAX;
	for (int i=0; i<r; i++){
		ns_parserr(&m, ns_s_an, i, &rr);
		ns_sprintrr(&m, &rr, NULL, NULL, buffer, BUFF_SIZE);
		cout << buffer << endl;
		parse_record(buffer, priority, address);
	}
	cout << priority << "\t" <<address << endl;

	hostent * record = gethostbyname(address);
	if (record==NULL) {
		cerr << "address not available"<<endl;
		return;
	}
	in_addr * IP = (in_addr*) record->h_addr;
	string IP_address = inet_ntoa(*IP);

	// prepare info
	set<string> rcpts; rcpts.insert(rcpt);
	string host_ip = IP_address + ":25";
	cout<<"IP_address = "<<host_ip<<endl;
	// send email to server
	int sent = sendto_smpt_server(host_ip, sender, rcpts, message.data);
	if (sent!=0){
		cout << "fails to send email" << endl;
		return;
	}

	// update mqueue
	cout << "email sent" <<endl;
	message.deleted = true;
}

void parse_record(char* buffer, int& priority, char* address){
	char* token = strtok(buffer, "\t");
	token = strtok(NULL, "\t");
	token = strtok(NULL, "\t");

	char* tok = strtok(token, " ");
	if (priority > atoi(tok)){
		priority = atoi(tok);
		tok = strtok(NULL, " ");
		strcpy(address, tok);
	}
}
