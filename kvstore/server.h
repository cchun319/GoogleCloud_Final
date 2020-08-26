/*
 * server.h
 *
 *  Created on: Apr 2, 2018
 *      Author: cis505
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <map>
#include "cell.h"
#include "filetool.h"

typedef std::map<std::string, Cell*> smap;
typedef std::map<std::string, smap > bmap;

#endif /* SERVER_H_ */
