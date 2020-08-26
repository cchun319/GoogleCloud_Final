/*
 * cell.h
 *
 *  Created on: 2018年4月10日
 *      Author: BearBurg
 */

#ifndef CELL_H_
#define CELL_H_

#include <string>
#include <iostream>
#include "filetool.h"

class Cell {
public:
	pthread_mutex_t cellMutex;
	std::string* contents;
	bool onDisk;

	Cell(std::string* s) {
		this->contents = s;
		pthread_mutex_init(&cellMutex, NULL);
		onDisk = false;
	}

	Cell(const Cell& other) {
		this->contents = other.contents;
		this->onDisk = other.onDisk;
		pthread_mutex_init(&cellMutex, NULL);
	}

	Cell() {
		pthread_mutex_init(&cellMutex, NULL);
		this->onDisk = false;
		this->contents = NULL;
	}

	~Cell() {
		std::cout << "destructor" <<std::endl;
		delete contents;
	}
};



#endif /* CELL_H_ */
