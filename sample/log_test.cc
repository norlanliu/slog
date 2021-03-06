/*
 * log_test.cpp
 *
 *  Created on: 2016��3��7��
 *      Author: edward
 */
#include <thread>
#include <vector>
#include <type_traits>
#include "../include/slogger.hpp"

using namespace slog;

void TestLogSystem() {
	LogToSystem(TRACE, "A system trace severity message %d", 666);
	LogToSystem(INFO, "A system INFO severity message %d", 667);
	LogToSystem(DEBUG, "A system DEBUG severity message %d", 668);
	LogToSystem(WARNING, "A system WARNING severity message %d", 669);
	LogToSystem(ERROR, "A system ERROR severity message %d", 670);
}

void TestLogDebug() {
	LogToDebug(TRACE, "A trace severity message %d", 666);
	LogToDebug(INFO, "A INFO severity message %d", 667);
	LogToDebug(DEBUG, "A DEBUG severity message %d", 668);
	LogToDebug(WARNING, "A WARNING severity message %d", 669);
	LogToDebug(ERROR, "A ERROR severity message %d", 670);
}

void LogFunc(std::string file_name) {
	Logger log = SetLogName(file_name);
	if(log) {
		LogToFile(log, TRACE, "A trace severity message %s", file_name.c_str());
		LogToFile(log, INFO, "A INFO severity message %s", file_name.c_str());
		LogToFile(log, DEBUG, "A DEBUG severity message %s", file_name.c_str());
		LogToFile(log, WARNING, "A WARNING severity message %s", file_name.c_str());
	}
}

void TestSystemRotation() {
	boost::this_thread::sleep(boost::posix_time::milliseconds(10));
	LogToSystem(TRACE, "A system trace severity message %d", 666);
	LogToSystem(INFO, "A system INFO severity message %d", 667);
	LogToSystem(DEBUG, "A system DEBUG severity message %d", 668);
	LogToSystem(WARNING, "A system WARNING severity message %d", 669);
	LogToSystem(ERROR, "A system ERROR severity message %d", 670);
}

void TestMultipleThreads() {
	const int num_threads = 5;
	std::vector<std::thread> threads;

	std::string log_prefix = "log";
	for(int i = 0; i < num_threads; i++) {
		std::string tmp(1, '0'+i);
		tmp = log_prefix + tmp;
		threads.push_back(std::thread(LogFunc, tmp));
	}

	for(int i = 0; i < num_threads; i++) {
		threads[i].join();
	}
}

int main(int, char*[]) {
	int success = InitLogWithLogName("Test", "test_dir", "system", "debug");
	if(success != 0) {
		std::cerr<<"Initialzed log error"<<std::endl;
		return -1;
	}
	TestLogSystem();
	TestLogDebug();
	TestMultipleThreads();
	TestSystemRotation();
	return 0;
}

