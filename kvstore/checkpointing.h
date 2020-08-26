/*
 * checkpointing.h
 *
 *  Created on: Apr 18, 2018
 *      Author: cis505
 */

#ifndef CHECKPOINTING_H_
#define CHECKPOINTING_H_

#include <string>
#include <sstream>
#include "server.h"
#include "serverState.h"

class CheckPointing {

public:

	CheckPointing();

	static bool writeToDisk(ServerState*);

	static bool recoverCheckpoint(ServerState*);

	static bool recoverLog(ServerState*);

	static long lastLogSeq(ServerState*);

	static long findVersionNum(ServerState*);

private:
	static bool replyCheckpointCommand(const std::stringstream& command);

	static bool replayLogCommand(const std::stringstream& command);

};


#endif /* CHECKPOINTING_H_ */
