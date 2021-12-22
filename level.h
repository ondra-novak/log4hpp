/*
 * level.h
 *
 *  Created on: 18. 12. 2021
 *      Author: ondra
 */

#ifndef LOG4HPP_LEVEL_H_
#define LOG4HPP_LEVEL_H_

namespace log4hpp {

namespace Level {
	using Type = unsigned int;
	///level disables logging complete
	constexpr Type nolevel = 0;
	///fatal error - probably can't continue
	constexpr Type fatal = 0x1000;
	///error, probably serious
	constexpr Type error = 0x2000;
	///warning, could be harmless
	constexpr Type warning = 0x3000;
	///important note, probably harmless
	constexpr Type note = 0x4000;
	///report progress of operation
	constexpr Type progress = 0x5000;
	///info level
	constexpr Type info = 0x6000;
	///debug level
	constexpr Type debug = 0x7000;
	///enables all levels
	constexpr Type max_verbose = 0xFFFF;
};

using ThreadId = unsigned int;


}



#endif /* LOG4HPP_LEVEL_H_ */
