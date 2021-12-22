/*
 * posix_file_appender.h
 *
 *  Created on: 21. 12. 2021
 *      Author: ondra
 */

#ifndef UNIX_FILE_APPENDER_H_
#define UNIX_FILE_APPENDER_H_

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

namespace log4hpp {


	class UnixFileAppender {
	public:

		UnixFileAppender(const std::string_view &pathname);
		~UnixFileAppender();

		void operator()(const std::string_view &line);

		void close();

	protected:
		std::string pathname;
		int fd = -1;
		std::size_t inst;
		std::mutex lock;

		bool open_file();
		std::size_t send(const std::string_view &line);

		void send_line_lk(const std::string_view &line);
	};



inline UnixFileAppender::UnixFileAppender(const std::string_view &pathname)
:pathname(pathname)
 ,fd(-1)
{
	if (!open_file()) {
		int e = errno;
		std::string msg("Can't open log file: ");
		msg.append(pathname);
		throw std::system_error(e,std::system_category(),msg);
	}
}


inline UnixFileAppender::~UnixFileAppender() {
	if (fd>=0) ::close(fd);
}

inline void UnixFileAppender::operator ()(const std::string_view &line) {
	std::lock_guard _(lock);
	send_line_lk(line);
}

inline void UnixFileAppender::send_line_lk(const std::string_view &line) {
	if (fd<0) {
		if (!open_file()) {
				return;
		}
	}
	auto sz = send(line);
	if (sz<line.size()) {
		if (sz) operator()(line.substr(sz));
	}
}

inline bool UnixFileAppender::open_file() {
	fd = ::open(pathname.c_str(), O_WRONLY|O_APPEND|O_CREAT|O_CLOEXEC|O_NONBLOCK, 0666);
	if (fd < 0) return false;
	struct stat st;
	if (!fstat(fd,&st)) {
		if ((st.st_mode & S_IFMT) != S_IFREG) {
			signal(SIGPIPE, SIG_IGN);
			int opt = fcntl(fd, F_GETFL);
			opt = opt & ~O_NONBLOCK;
			fcntl(fd, F_SETFL,  opt);
		}
	}
	return true;
}

inline void UnixFileAppender::close() {
	std::lock_guard _(lock);
	if (fd>=0) {
		::close(fd);
		fd = -1;
	}
}

inline std::size_t UnixFileAppender::send(const std::string_view &line) {
	int s = ::write(fd, line.data(), line.size());
	if (s <= 0) {
		::close(fd);
		fd = -1;
		return 0;
	} else  {
		if (static_cast<std::size_t>(s) < line.size())
			return s+send(line.substr(s));
		else
			return s;
	}
}


}


#endif /* UNIX_FILE_APPENDER_H_ */
