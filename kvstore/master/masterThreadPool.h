/*
 * masterThreadPool.h
 *
 *  Created on: Feb 27, 2018
 *      Author: cis505
 */

#ifndef masterThreadPool_H_
#define masterThreadPool_H_

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <set>
#include <list>
#include <string.h>

#include "../master/masterConnection.h"
#include "../master/masterWorker.h"
using namespace std;

class masterThreadPool{


private:
	static list<masterConnection*> workQueue;
	static pthread_mutex_t mutex;
	static pthread_cond_t  condv;

	static set<masterConnection*> ongoing;

	//static MyQueue tasks;
	static bool closed;
	int thread_num;
	static bool v_flag;

	pthread_t* t_ids;

	static bool is_quit_command(char* string);
	static int strcmpi(const char* s1, const char* s2);
	static int extract_command(char* buf, char* command);
	static void run(masterConnection* task);

public:
	masterThreadPool(int);

	void setFlag(bool);
	int CreatePool();
	static void* ThreadFun(void *);
	int addTask(masterConnection*);
	int closeAll();
	int size();

};



#endif /* masterThreadPool_H_ */
