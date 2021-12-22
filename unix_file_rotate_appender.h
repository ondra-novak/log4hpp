/*
 * unix_file_rotate_appender.h
 *
 *  Created on: 22. 12. 2021
 *      Author: ondra
 */

#ifndef LOG4HPP_UNIX_FILE_ROTATE_APPENDER_H_
#define LOG4HPP_UNIX_FILE_ROTATE_APPENDER_H_


#include <cstring>
#include <dirent.h>
#include <algorithm>
#include "unix_file_appender.h"

namespace log4hpp {

class UnixFileRotatedAppender: public UnixFileAppender {
public:
	UnixFileRotatedAppender(const std::string_view &pathname, unsigned long days = 7, unsigned long day_seconds = 24*60*60, const std::string_view &dateformat="%Y%m%d");

	void operator()(const std::string_view &line);

protected:
	unsigned long days;
	unsigned long day_seconds;
	unsigned long cur_day;
	std::string dateformat;

	void do_rotate(std::time_t tm);

};

log4hpp::UnixFileRotatedAppender::UnixFileRotatedAppender(
		const std::string_view &pathname, unsigned long days, unsigned long day_seconds,const std::string_view &dateformat)
:UnixFileAppender(pathname),days(days),day_seconds(day_seconds), cur_day(0),dateformat(dateformat)
{
}

inline void log4hpp::UnixFileRotatedAppender::operator ()(const std::string_view &line) {
	std::lock_guard _(lock);
	auto now = std::time(nullptr);
	unsigned long day = now/day_seconds;
	if (day != cur_day) {
		do_rotate(now - day_seconds);
		cur_day = day;
	}
	send_line_lk(line);
}

inline void log4hpp::UnixFileRotatedAppender::do_rotate(std::time_t tm) {
	std::string name;
	name.append(pathname);
	name.push_back('-');
	auto pos = name.size();
	name.resize(pos+5*dateformat.size());
	std::strftime(name.data()+pos, name.size()-pos, dateformat.c_str(), gmtime(&tm));
	if (access(name.c_str(),F_OK) == 0) return;
	close();
	rename(pathname.c_str(), name.c_str());
	if (days > 0) {
		auto sep = name.rfind('/');
		std::string base = pathname.substr(sep+1);
		if (sep != name.npos) name.resize(sep);
		DIR *d = opendir(name.c_str());
		if (d) {
			try {
				std::vector<std::pair<time_t,std::string> > files;
				struct dirent *entry;
				while ((entry=readdir(d)) != nullptr) {
					std::string_view ename(entry->d_name, strlen(entry->d_name));
					if (ename.substr(0, base.size()) == base) {
						name.push_back('/');
						name.append(ename);
						struct stat st;
						if (!stat(name.c_str(), &st)) {
							files.emplace_back(st.st_mtim.tv_sec, name);
						}
						name.resize(sep);
					}
				}
				if (files.size() > days) {
					std::sort(files.begin(), files.end());
					files.resize(files.size()-days);
					for (const auto &c: files) {
						unlink(c.second.c_str());
					}
				}

			} catch(...) {
				//empty;
			}
		}
		closedir(d);
	}

}


}



#endif /* LOG4HPP_UNIX_FILE_ROTATE_APPENDER_H_ */
