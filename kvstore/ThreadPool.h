/*
 * ThreadPool.h
 *
 *  Created on: Feb 27, 2018
 *      Author: cis505
 */

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <set>
#include <list>
#include <string.h>

#include "Connection.h"
#include "Worker.h"
#include "filetool.h"
using namespace std;

class ThreadPool{


private:
	static list<Connection*> workQueue;
	static pthread_mutex_t mutex;
	static pthread_cond_t  condv;

	static set<Connection*> ongoing;

	//static MyQueue tasks;
	static bool closed;
	int thread_num;
	static bool v_flag;

	pthread_t* t_ids;

	static bool is_quit_command(char* string);
	static int strcmpi(const char* s1, const char* s2);
	static int extract_command(char* buf, char* command);
	static void run(Connection* task);

public:
	ThreadPool(int);

	void setFlag(bool);
	int CreatePool();
	static void* ThreadFun(void *);
	int addTask(Connection*);
	int closeAll();
	int size();

};



#endif /* THREADPOOL_H_ */
