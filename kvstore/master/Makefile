TARGETS = masterserver

all: $(TARGETS)

#masterConnection.o: masterConnection.cc
#	g++ -c $^ -std=c++11
#	
#masterWorker.o: masterWorker.cc
#	g++ -c $^ -std=c++11
#	
#masterThreadPool.o: masterThreadPool.cc
#	g++ -c $^ -std=c++11 -lpthread
	
masterserver: master.cc masterConnection.cc masterThreadPool.cc masterWorker.cc ../filetool.cc
	g++ -o $@ $^  -std=c++11 -lpthread -lcrypto -g 

pack:
	rm -f submit-hw2.zip
	zip -r submit-hw2.zip *.cc *.h README Makefile

clean::
	rm -fv $(TARGETS) *~

realclean:: clean
	rm -fv cis505-hw2.zip
