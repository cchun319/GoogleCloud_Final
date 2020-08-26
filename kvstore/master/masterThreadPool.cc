/*
 * masterThreadPool.cc
 *
 *  Created on: Feb 27, 2018
 *      Author: cis505
 */

#include "../master/masterThreadPool.h"

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <list>
#include <set>
#include <pthread.h>

#include "../master/masterConnection.h"
#include "../master/masterWorker.h"

using namespace std;

list<masterConnection*> masterThreadPool::workQueue;
pthread_mutex_t masterThreadPool::mutex;
pthread_cond_t masterThreadPool::condv;
set<masterConnection*> masterThreadPool::ongoing;
bool masterThreadPool::v_flag = false;
bool masterThreadPool::closed = false;

masterThreadPool::masterThreadPool(int num) {
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&condv, NULL);

	this->thread_num = num;
	t_ids = NULL;
}

int masterThreadPool::CreatePool() {
	t_ids = (pthread_t*) malloc(sizeof(pthread_t) * thread_num);
	for (int i = 0; i < thread_num; i++) {
		pthread_create(&t_ids[i], NULL, ThreadFun, NULL);
	}
	return 0;
}

void masterThreadPool::setFlag(bool flag) {
	v_flag = flag;
}

int masterThreadPool::addTask(masterConnection* task) {
	pthread_mutex_lock(&mutex);

	workQueue.push_back(task);
	pthread_cond_signal(&condv);

	pthread_mutex_unlock(&mutex);
	return 0;
}

int masterThreadPool::closeAll() {
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

void* masterThreadPool::ThreadFun(void* arg) {
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

		masterConnection* task = workQueue.front();
		workQueue.pop_front();
		//cout<< "Thread "<< tid << " starts working" << endl;

		pthread_mutex_unlock(&mutex);

		ongoing.insert(task);

		masterWorker* worker = new masterWorker(task, v_flag);
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

int masterThreadPool::size() {
	return workQueue.size();
}


