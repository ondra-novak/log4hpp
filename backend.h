/*
 * backend.h
 *
 *  Created on: 18. 12. 2021
 *      Author: ondra
 */

#ifndef LOG4HPP_BACKEND_H_
#define LOG4HPP_BACKEND_H_

#include <string_view>
#include <memory>
#include "level.h"

namespace log4hpp {

struct ThreadContext;
struct StaticContext;

class AbstractContext;

///processes collected message, appends, format, additional filtering, sending to logger thread
class IBackend {
public:

	///Receives message from logger
	/**
	 * @param thr Thread context
	 * @param level message level
	 * @param context recursive context information - note it cannot be used asynchronously, pointer
	 * is valid only during this call
	 * @param message message to process - note string is valid only during this call, otherwise must
	 * be copied
	 */
	virtual void send(ThreadContext &thr, Level::Type level, const AbstractContext *context, const std::string_view &message) = 0;
	///Send line directly to the appender with no futher processing
	/** @param line line to send */
	virtual void direct_send(const std::string_view &line) = 0;
	virtual Level::Type getLevel() const = 0;
	virtual ~IBackend() {}

};



template<typename Appender>
class Backend: public IBackend {
public:

	template<typename ... Args>
	Backend(const std::string_view &format, Level::Type level, Args && ... appender)
		:format(format),level(level),appender(std::forward<Args>(appender)...) {}


	void initCounter(std::size_t cnt) {this->msgcnt = cnt;}

	virtual void send(ThreadContext &thr, Level::Type level, const AbstractContext *context, const std::string_view &message);
	virtual void direct_send(const std::string_view &line) {appender(line);}
	virtual Level::Type getLevel() const {
		return level;
	}

	Appender *operator->() {
		return &appender;
	}
	const Appender *operator->() const {
		return &appender;
	}


protected:

	std::string format;
	Level::Type level;
	Appender appender;
	std::atomic<std::size_t> msgcnt;
};

template<typename Fn>
std::shared_ptr<IBackend> makeBackend(const std::string_view &format, Level::Type level, Fn &&appender) {
	return std::make_shared<Backend<Fn> >(format, level, std::forward<Fn>(appender));
}



}

#endif /* LOG4HPP_BACKEND_H_ */
