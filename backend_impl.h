/*
 * backend_impl.h
 *
 *  Created on: 20. 12. 2021
 *      Author: ondra
 */

#ifndef BACKEND_IMPL_H_
#define BACKEND_IMPL_H_

#include "context.h"
#include "backend.h"

namespace log4hpp {


static inline void outLevelName(int level, Buffer &out) {
	std::string_view levelName[] = {
			"","FATAL","ERROR","WARN","NOTE","PROGR","INFO","DEBUG"
	};
	auto select = levelName[(level >> 4)&0x7];
	out.append(select);
	auto sub = level & 0xF;
	if (sub) {
		out.push_back('/');
		out.push_back('0'+(sub/10));
		out.push_back('0'+(sub%10));
	}
}



template<typename Appender>
inline void BackendT<Appender>::send(ThreadContext &thr,
							Level::Type level, const AbstractContext *context,
							const std::string_view &message) {
	Buffer &buffer = thr.fmt_buffer;
	std::time_t tm = std::time(nullptr);

	auto smap = [&](const std::string_view &type, auto &&out) {
		if (!type.empty()) {
			switch(type[0]) {
			case 't': {
				buffer.clear();
				if (type.length()>1) {
					auto fmt = type.substr(1);
					if (fmt.length()>1 && fmt[0] == '[' && fmt[fmt.size()-1] == ']') {
						fmt = fmt.substr(1, fmt.size()-2);
					}
					auto needsz = fmt.length()*5;
					buffer.resize(needsz);
					buffer.append(fmt);
					auto cnt = std::strftime(buffer.data(),needsz,buffer.data()+needsz, gmtime(&tm));
					buffer.resize(cnt);
				} else {
					buffer.resize(50);
					auto cnt = std::strftime(buffer.data(),50,"%FT%TZ" , gmtime(&tm));
					buffer.resize(cnt);
				}
				out(buffer);
			}break;
			case 'c': {
				if (type == "cr") {
					out(DirectString("\r"));
				} else {
					buffer.clear();
					if (context) {
						context->walk([&](const AbstractContext *c){
							c->toString(buffer);
							if (c != context) buffer.append(type.substr(1));
						});
					}
					out(buffer);
				}
			}break;
			case 'C': {
				buffer.clear();
				auto x = context;
				while (x) {
					x->toString(buffer);
					x = x->getPrevContext();
					if (x) buffer.append(type.substr(1));
				}
				out(buffer);
			}break;
			case 'T':
				out(thr.threadId);
				break;
			case 'N':
				out(++msgcnt);
				break;
			case 'm':
				out(message);
				break;
			case 'L':
				buffer.clear();
				outLevelName(level>>8, buffer);
				out(buffer);
				break;
			case 'l':
				if (type == "lf") out(DirectString("\n"));
				else {
					buffer.clear();
					outLevelName(level & 0xFF, buffer);
					out(buffer);
				}
				break;
			case 'k':
				out(level);
				break;
			case 'n':
				if (type == "nl") out(DirectString(
						#ifdef _WIN32
							"\r\n"
						#elif defined macintosh // OS 9
							"\r"
						#else
							"\n"
						#endif
						));
				break;
			}
		}
	};

	Buffer &out = thr.bk_buffer;
	out.clear();
	FormatT<Buffer &,decltype(smap)> fmt(out, std::move(smap));
	fmt(format);
	appender(out);
}

inline std::shared_ptr<IBackend> setActiveInThread(std::shared_ptr<IBackend> newBk) {
	auto &ts = ThreadContext::current();
	auto cur = ts.backend;
	ts.backend = newBk;
	return cur;

}

template<typename Appender>
inline void Backend<Appender>::install() {
	auto &gs = GlobalContext::current();
	gs.backend = ptr;

}

template<typename Appender>
inline std::shared_ptr<IBackend> Backend<Appender>::setActive() {
	return setActive(ptr);
}

template<typename Appender>
inline std::shared_ptr<IBackend> Backend<Appender>::setActive(std::shared_ptr<IBackend> bk) {
	auto &ts = ThreadContext::current();
	auto cur = ts.backend;
	ts.backend = bk;
	return cur;

}


}




#endif /* BACKEND_IMPL_H_ */
