TARGETS = frontend load_balancer

all: $(TARGETS)


frontend: frontend.cc request.cc response.cc util.cc ../storage/Storage.cc ../storage/fileHandler.cc ../storage/masterConnect.cc ../webmail/client.cc ../webmail/utils.cc ../state/StateCheck.cc
	g++ $^ -I/usr/local/opt/openssl/include -L/usr/local/opt/openssl/lib -lcrypto -lpthread -g -o $@

load_balancer: load_balancer.cc util.cc
	g++ $^ -I/usr/local/opt/openssl/include -L/usr/local/opt/openssl/lib -lcrypto -lpthread -g -o $@

clean::
	rm -fv $(TARGETS) *~

