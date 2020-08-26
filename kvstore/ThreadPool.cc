/*
 * ThreadPool.cc
 *
 *  Created on: Feb 27, 2018
 *      Author: cis505
 */

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <list>
#include <set>
#include <pthread.h>

#include "Connection.h"
#include "ThreadPool.h"
#include "Worker.h"
#include "filetool.h"

using namespace std;

list<Connection*> ThreadPool::workQueue;
pthread_mutex_t ThreadPool::mutex;
pthread_cond_t ThreadPool::condv;
set<Connection*> ThreadPool::ongoing;
bool ThreadPool::v_flag = false;
bool ThreadPool::closed = false;

ThreadPool::ThreadPool(int num) {
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&condv, NULL);

	this->thread_num = num;
	t_ids = NULL;
}

int ThreadPool::CreatePool() {
	t_ids = (pthread_t*) malloc(sizeof(pthread_t) * thread_num);
	for (int i = 0; i < thread_num; i++) {
		pthread_create(&t_ids[i], NULL, ThreadFun, NULL);
	}
	return 0;
}

void ThreadPool::setFlag(bool flag) {
	v_flag = flag;
}

int ThreadPool::addTask(Connection* task) {
	pthread_mutex_lock(&mutex);

	workQueue.push_back(task);
	pthread_cond_signal(&condv);

	pthread_mutex_unlock(&mutex);
	return 0;
}

int ThreadPool::closeAll() {
	if (closed) {
		return -1;
	}

	closed = true;
	int ret = pthread_cond_broadcast(&condv);

	for (int i = 0; i < thread_num; i++) {
		pthread_detach(t_ids[i]);
		pthread_cancel(t_ids[i]);
	}

	for (auto task : ongoing) {
		string error_message = "-ERR Server shutting down \r\n";
		task->do_write(error_message.c_str(), error_message.length());
		delete task;
	}

	printf("All closed\n");

	free(t_ids);

	return 0;

}

void* ThreadPool::ThreadFun(void* arg) {
	pthread_t tid = pthread_self();

	while (true) {
		//cout<< "Thread "<< tid << " waiting for work" << endl;

		pthread_mutex_lock(&mutex);

		while (workQueue.size() == 0 && !closed) {
			pthread_cond_wait(&condv, &mutex);
		}

		if (closed) {
			pthread_mutex_unlock(&mutex);
			break;
		}

		Connection* task = workQueue.front();
		workQueue.pop_front();
		//cout<< "Thread "<< tid << " starts working" << endl;

		pthread_mutex_unlock(&mutex);

		ongoing.insert(task);

		Worker* worker = new Worker(task, v_flag);
		worker->run();

		if (closed) {
			worker->closeWorker();
			break;
		}

		ongoing.erase(task);
		delete task;
		delete worker;

	}

	return NULL;
}

int ThreadPool::size() {
	return workQueue.size();
}


