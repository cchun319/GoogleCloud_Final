TARGETS = server client_test

all: $(TARGETS)

server: server.cc utils.cc deliver.cc client.cc
	g++ $^ -I/usr/local/opt/openssl/include -L/usr/local/opt/openssl/lib -lcrypto -lresolv -lpthread -g -o $@
	
client_test: client_test.cc utils.cc client.cc
	g++ $^ -I/usr/local/opt/openssl/include -L/usr/local/opt/openssl/lib -lcrypto -lpthread -g -o $@
    
clean::
	rm -fv $(TARGETS) *.o *~
