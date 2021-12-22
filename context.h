/*
 * context.h
 *
 *  Created on: 18. 12. 2021
 *      Author: ondra
 */

#ifndef LOG4HPP_CONTEXT_H_
#define LOG4HPP_CONTEXT_H_

#include <mutex>
#include <atomic>
#include <tuple>
#include <vector>

#include "format.h"
#include "backend.h"

#include "level.h"
namespace log4hpp {

class Buffer {
public:

	void operator()(char c) {_data.push_back(c);}
	void clear() {_data.clear();}
	auto data() {return _data.data();}
	std::size_t size() const {return _data.size();}
	void resize(std::size_t sz) {_data.resize(sz);}
	void push_back(char c) {_data.push_back(c);}
	void append(const std::string_view &txt) {for (char c: txt) _data.push_back(c);}
	void append(const char *c, std::size_t sz) {for (std::size_t i = 0; i < sz; i++) _data.push_back(c[i]);}
	operator std::string_view() const {return std::string_view(_data.data(), _data.size());}

protected:
	std::vector<char> _data;
};

template<> class Stringify<Buffer>: public StringifyString {};

struct GlobalContext {
	///Thread identifier (each new thread allocates new ID)
	std::atomic<unsigned int> threadCounter;
	///current backend
	std::shared_ptr<IBackend> backend;

	static GlobalContext& current() {
		static GlobalContext st;
		return st;
	}

	GlobalContext() {}
	GlobalContext(const GlobalContext &) = delete;
	GlobalContext &operator=(const GlobalContext &) = delete;

};


class AbstractContext;

using LevelTransformFn = Level::Type (*)(Level::Type, void *);


struct ThreadContext {
	///current loggin level for thread
	Level::Type level;
	///current thread id
	unsigned int threadId;
	///buffer for formatting message before it is send to the backend
	Buffer buffer;
	///secondary buffer - backend will use this buffer to build final line
	Buffer bk_buffer;
	///third buffer - backend will use the buffer to store temporary strings there
	Buffer fmt_buffer;

	AbstractContext *curCtx = nullptr;
	///current backend
	std::shared_ptr<IBackend> backend;


	ThreadContext(GlobalContext &st){
		level = st.backend->getLevel();
		threadId = st.threadCounter++;
		backend = st.backend;	}

	static ThreadContext &current() {
		thread_local ThreadContext th(GlobalContext::current());
		return th;
	}

	ThreadContext(const ThreadContext &) = delete;
	ThreadContext &operator=(const ThreadContext &) = delete;

	Level::Type transform(Level::Type level) const;

};



class IContext {
public:
	virtual ~IContext() {}
	///Appends context description to the output string
	/** function must not clear the buffer */
	virtual void toString(Buffer &out) const = 0;

};

class Attach;

///Abstract context must be instancied at stack;
class AbstractContext: public IContext {
public:


	explicit AbstractContext(ThreadContext *ctx):current(ctx) {
		prevContext = current->curCtx;
		current->curCtx = this;
	}

	AbstractContext():AbstractContext(&ThreadContext::current()) {}

	AbstractContext(const AbstractContext &other) = delete;
	AbstractContext &operator=(const AbstractContext &other) = delete;

	virtual ~AbstractContext() {
		if (current) {
			current->curCtx = prevContext;
		}
	}

	void setLevel(Level::Type level) {
		if (level < current->level) current->level = level;
	}


	const AbstractContext *getPrevContext() const {return prevContext;}
	template<typename Fn>
	void walk(Fn &&fn) const {
		if (prevContext) prevContext->walk(std::forward<Fn>(fn));
		fn(this);
	}



	template<typename ... Args>
	inline void log(Level::Type level, const std::string_view &msg, const Args & ... args) {
		if (current->level >= level) {
			current->buffer.clear();
			FormatT<Buffer &,NullMap> fmt(current->buffer);
			fmt(msg, args...);
			current->backend->send(*current, level, this, current->buffer);
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
		log(Level::note, msg, args...);
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

protected:


	AbstractContext *prevContext=nullptr;
	ThreadContext *current = nullptr;
	LevelTransformFn trnfn = nullptr;
	void *trnptr = nullptr;

	void detach() {
		if (current) {
			current->curCtx = prevContext;
			current = nullptr;
			prevContext = nullptr;
		}
	}

	void attach(ThreadContext *ctx) {
		detach();
		current = ctx;
		prevContext = current->curCtx;
		current->curCtx = this;
	}

	void attach() {
		attach(&ThreadContext::current());
	}

	friend class Attach;
	friend struct ThreadContext;

};

class Attach {
public:

	Attach(AbstractContext &ctx):ctx(ctx) {
		ctx.attach();
	}
	~Attach() {
		ctx.detach();
	}
	template<typename ... Args>
	inline void log(Level::Type level, const std::string_view &msg, const Args & ... args) {
		ctx.log(level, msg, args...);
	}

	Attach(const Attach &) = delete;
	Attach &operator=(const Attach &) = delete;

protected:
	AbstractContext &ctx;
};

inline Level::Type ThreadContext::transform(Level::Type t) const {
	if (curCtx && curCtx->trnfn) return curCtx->trnfn(t,curCtx->trnptr);
	else return t;
}


template<typename StrType, typename ... Args>
class FmtContext {
public:
	FmtContext(ThreadContext *ctx, StrType &&format, Args && ... args)
			:str(std::forward<StrType>(format))
			,args(std::forward<Args>(args)...) {}

	virtual void toString(Buffer &out) {
		FormatT<Buffer &, NullMap> fmt(out);
		std::apply([&](const auto &... args ){
			fmt(str, args...);
		}, args);
	}

protected:
	StrType str;
	std::tuple<Args...> args;


};

template<typename ... Args>
auto makeContext(const std::string_view &format, const Args & ... args) {
	return FmtContext<std::string_view, const Args & ...>(&ThreadContext::current(), format, args...);
}
template<typename ... Args>
auto makeDetachedContext(const std::string_view &format, const Args & ... args) {
	return FmtContext<std::string, Args ...>(nullptr, format, args...);
}

inline Buffer &getTmpBuffer() {
	return ThreadContext::current().buffer;
}

}



#endif /* LOG4HPP_CONTEXT_H_ */
