TARGETS = dataserver

all: $(TARGETS)

#Connection.o: Connection.cc
#	g++ -c $^ -std=c++11
#	
#Worker.o: Worker.cc
#	g++ -c $^ -std=c++11
#	
#ThreadPool.o: ThreadPool.cc
#	g++ -c $^ -std=c++11 -lpthread
#	
#filetool.o:filetool.cc
#	g++ -c $^ -std=c++11
	
dataserver: dataserver.cc Connection.cc ThreadPool.cc Worker.cc filetool.cc backStore.cc serverState.cc checkpointing.cc
	g++ $^ -pthread -std=c++11 -o $@

pack:
	rm -f submit-hw2.zip
	zip -r submit-hw2.zip *.cc *.h README Makefile

clean::
	rm -fv $(TARGETS) *~

realclean:: clean
	rm -fv cis505-hw2.zip
