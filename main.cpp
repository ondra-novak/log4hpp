/*
 * main.cpp
 *
 *  Created on: 18. 12. 2021
 *      Author: ondra
 */

#include <iostream>
#include <thread>
#include "logger.h"
#include "unix_file_rotate_appender.h"
#include "stderr_appender.h"

template<typename T> struct PSpec;



int main(int argc, char **argv) {

	auto backend = std::make_shared<log4hpp::Backend<log4hpp::UnixFileRotatedAppender> >(
			"{N:8A} {t[%d.%m.%Y %H:%M:%S]} {L} {c} {m:qd}{nl}",
			log4hpp::Level::debug, "log/logfile", 7, 60, "%Y%m%d%H%M%S");
log4hpp::setActive(backend);

for (int i = 0; i< 10000; i++) {
	log::debug("Hodnota je {} - text {}", i,"Ahoj \"Světe\"");
	std::this_thread::sleep_for(std::chrono::seconds(1));
}

}


