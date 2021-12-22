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

	//Create logging backend
	//Backend is object which routes messages to an Appender
	//Appender is object constructed by the backend, you specify appended as template argument
	//Constructor parameters are passed through the constructor of the backend
	log4hpp::Backend<log4hpp::UnixFileRotatedAppender> logBackend(
			"{N:8A} {t} {L} {c} {m}{nl}", //format of log message
			      //{N} - id of message
			      //{t} - timestamp
				  //{L} - main level
			      //{c} - context
			      //{m} - log message
			      //{nl} - new line character(s)
			log4hpp::Level::debug,    //maximum level of detail
			"log/logfile",            //target file
			7);                       //days to keep - log is rotated automatically
	logBackend.install();    //install the backend

	//backend is installed for the current thread and for every newly created thread.
	//currently running threads are not affected, they can run any backend installed before.
	//New backend must be reactivated manually by calling setActive()
	//
	//You can also have different backends for different thread.

	for (int i = 0; i< 10; i++) {
		log::debug("Hodnota je {} - text {}", i,"Ahoj \"Světe\"");
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

}


