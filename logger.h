/*
 * logger.h
 *
 *  Created on: 19. 12. 2021
 *      Author: ondra
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <typeinfo>
#include "format.h"
#include "backend_impl.h"

namespace log {

namespace Level {
using namespace log4hpp::Level;
}

template<typename ... Args>
inline void log(log4hpp::Level::Type level, const std::string_view &msg, const Args & ... args) {
	using namespace log4hpp;
	ThreadContext *current = &ThreadContext::current();
	if (current->level >= level) {
		current->buffer.clear();
		FormatT<Buffer &, NullMap> fmt(current->buffer);
		fmt(msg, args...);
		current->backend->send(*current, level, current->curCtx, current->buffer);
	}
}

template<typename ... Args>
inline void debug(const std::string_view &msg, const Args & ... args) {
	log(Level::debug, msg, args...);
}
template<typename ... Args>
inline void info(const std::string_view &msg, const Args & ... args) {
	log(Level::info, msg, args...);
}
template<typename ... Args>
inline void note(const std::string_view &msg, const Args & ... args) {
	log(log4hpp::Level::note, msg, args...);
}
template<typename ... Args>
inline void progress(const std::string_view &msg, const Args & ... args) {
	log(Level::progress, msg, args...);
}
template<typename ... Args>
inline void warning(const std::string_view &msg, const Args & ... args) {
	log(Level::warning, msg, args...);
}
template<typename ... Args>
inline void error(const std::string_view &msg, const Args & ... args) {
	log(Level::error, msg, args...);
}
template<typename ... Args>
inline void fatal(const std::string_view &msg, const Args & ... args) {
	log(Level::fatal, msg, args...);
}

using log4hpp::makeContext;
using log4hpp::makeDetachedContext;
using log4hpp::makeBackend;


}



#endif /* LOGGER_H_ */
