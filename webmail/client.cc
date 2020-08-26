#include "client.h"

const int BUFF_SIZE = 100;
const bool DEBUG = true;

int sendto_smpt_server(string& server_address, string& sender, set<string>& rcpts, string& data){
	// create a new socket(TCP)
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0){
		cerr << "cannot open socket\r\n";
		return 1;
	}
	// set port for reuse
	const int REUSE = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &REUSE, sizeof(REUSE));

	// parse address and connect to server
	struct sockaddr_in dest;
	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_INET;

	// parse smpt server address
	size_t found = server_address.find(':');
	string ip = server_address.substr(0, found);
	string port = server_address.substr(found+1);
	inet_pton(AF_INET, ip.c_str() , &dest.sin_addr);
	dest.sin_port = htons(atoi(port.c_str()));

	// request and response
	char response[BUFF_SIZE];
	string request;

	int con = connect(sock, (struct sockaddr*)&dest, sizeof(dest));
	if (con<0){
		cerr << "cannot connect to server\r\n";
		return 2;
	}
	read(sock, response, BUFF_SIZE);

	request = "HELO tester\r\n";
	handle_comm(sock, request, response);

	request = "MAIL FROM:<" + sender + ">\r\n";
	handle_comm(sock, request, response);

	for (auto rcpt: rcpts){
		request = "RCPT TO:<" + rcpt + ">\r\n";
		handle_comm(sock, request, response);
	}

	request = "DATA\r\n";
	handle_comm(sock, request, response);

	// follow smpt protocol, send content line by line
	size_t next; int prev = -2;
	while (true){
		next = data.find("\r\n", prev+2);
		if (next==string::npos) break;

		request = data.substr(prev+2, next-prev);
		do_write(sock, request.c_str(), request.length());

		prev = next;
	}
	cout << data;

	// end of data
	request = ".\r\n";
	handle_comm(sock, request, response);

	// close session in smtp server
	request = "QUIT\r\n";
	handle_comm(sock, request, response);

	close(sock);
	return 0;
}

void handle_comm(int& sock, string& request, char* response){
	// communication between SMTP client and server
	do_write(sock, request.c_str(), request.length());
	memset(response, 0, BUFF_SIZE);

	int len = 0;
	while(len<BUFF_SIZE){
		int n = read(sock, response, BUFF_SIZE-len);
		len += n;
		if (strstr(response,"\r\n")!=NULL) break;
	}

	if (DEBUG) {
		cout << "request: " << request;
		cout << "respond: " << response;
	}
}

vector<email> recvfrom_backend(string& master_address, string& user, bool inbox){ // load all emails in user's inbox or outbox
	string message;
	string response;
	string node_address;
	vector<email> emails;

	string prefix; // prefix for inbox and outbox
	if(inbox) prefix = "mail/inbox/";
	else prefix = "mail/outbox/";

	// ask master about node address
	node_address = ask_master(master_address, user);
	if (node_address.compare("NULL")==0) {
		cerr << "ERR: user does not exist"<<endl;
		return emails;
	}

	// get mail hash list
	message = "GET <" + user + "> <" + prefix + "list>\r\n";
	response = sendto_backend(node_address, message.c_str());
	if (response.compare(1,3, "ERR")==0) return emails; //empty list

	// get each mail
	istringstream ss (response); string hash;
	while(getline(ss, hash)){
		if (hash.compare(0,4,"list")==0) continue;
		hash.pop_back(); // remove \n
		string column_key = prefix + hash;
		email mail = extract_email(node_address, user, column_key);
		emails.push_back(mail);
	}
	return emails;
}

email extract_email(string& node_address, string& user, string& column_key){
	string message = "GET <" + user + "> <" + column_key + ">\r\n";
	string response = sendto_backend(node_address, message.c_str());

	// parse email
	size_t f1 = response.find("Header ");
	size_t f2 = response.find("\r\n", f1+1);
	string header = response.substr(f1, f2-f1);

	// generate hash
	string content = response.substr(f2+2);
	string hash = hash_MD5(content);

	// content will skip subject line
	size_t f3 = response.find("\r\n", f2+1);
	content = response.substr(f3+2);

    return email(header, content, hash);
}

void update_backend(string& master_address, string& user, set<string> delete_hash, bool inbox){ // delete user's emails with hash
	string message;
	string response;
	string value;
	string node_address;

	string prefix; // prefix for inbox and outbox
	if(inbox) prefix = "mail/inbox/";
	else prefix = "mail/outbox/";

	// ask master about node address
	node_address = ask_master(master_address, user);
	if (node_address.compare("NULL")==0) {
		cerr << "ERR: user does not exist"<<endl;
		return;
	}

	// update hash list
    while (true){
	    message = "GET <" + user + "> <" + prefix + "list>\r\n";
	    response.clear();
	    response = sendto_backend(node_address, message.c_str());

	    value = update_hash(response, delete_hash);
	    if (value.compare("list\r\n")==0){ // nothing but initial mark
	    	message = "DELETE <" + user + "> <" + prefix + "list>\r\n";
	    	response.clear();
	        response = sendto_backend(node_address, message.c_str());
	        break;
	    }

	    message = "CPUT <" + user + "> <" + prefix + "list>\r\nOLD\r\n" + response + ".\r\nNEW\r\n" + value +".\r\n";
	    response.clear();
    	response = sendto_backend(node_address, message.c_str());
    	cerr << "update hash list: " << response << endl;
    	if (response.compare(1,2, "OK")==0) break;
    }

	// delete emails
	for (auto& hash: delete_hash){
		message = "DELETE <" + user + "> <"+ prefix + hash +">\r\n";
		response = sendto_backend(node_address, message.c_str());
		cerr << "delete email: " << response << endl;
	}

}

string update_hash(const string& old_hash, set<string> delete_hash){
	// remove hash to delete in hash list
	string new_hash;
	istringstream ss(old_hash);
	string line;

	while(getline(ss, line)){
		line.pop_back();
		if(delete_hash.find(line)==delete_hash.end()){
			// not deleted
			line += "\r\n";
			new_hash += line;
		}
	}
    return new_hash;
}
