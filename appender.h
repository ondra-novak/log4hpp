/*
 * appender.h
 *
 *  Created on: 20. 12. 2021
 *      Author: ondra
 */

#ifndef LOG4HPP_APPENDER_H_
#define LOG4HPP_APPENDER_H_

namespace log4hpp {


///handles appending lines to the log
/**
 * Default format expects, that log consists from lines
 */
class IAppender {
public:

	virtual ~IAppender() {}

	virtual void append(const std::string_view &line) = 0;


};

}




#endif /* LOG4HPP_APPENDER_H_ */
