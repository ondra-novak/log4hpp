/*
 * stderr_appender.h
 *
 *  Created on: 21. 12. 2021
 *      Author: ondra
 */

#ifndef STDERR_APPENDER_H_
#define STDERR_APPENDER_H_

#include <mutex>
#include <iostream>

namespace log4hpp {


class StdErrAppender {
public:
	void operator()(std::string_view line) {
		std::lock_guard _(lock);
		std::cerr << line;
	}
protected:
	std::mutex lock;


};

}

#endif /* STDERR_APPENDER_H_ */
