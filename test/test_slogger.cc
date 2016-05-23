#include <fstream>
#include <algorithm>
#include <iterator>
#include <thread>
#include <vector>
#include <type_traits>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include "slogger.hpp"
#include "gtest/gtest.h"
using namespace slog;

const std::string log_dir = "./test_logs";
const std::string system_log_name = "system";
const std::string debug_log_name = "debug";
const std::string log_file_name = "particular.log";
const std::string log_file_prefix = "log";

const char* ROTATE_FILE_NAME_FORMAT = "_%Y%m%d_";

class CorrectnessTest : public testing::Test {
	protected:
		static void SetUpTestCase() {
			filesystem::path log_path(log_dir);
			if(boost::filesystem::exists(log_path)) {
				boost::filesystem::remove_all(log_path);
			}
			ASSERT_EQ(0, InitLogWithLogName("Test", log_dir, system_log_name, debug_log_name));
		}
		static void TearDownTestCase() {
			filesystem::path log_path(log_dir);
			if(boost::filesystem::exists(log_path)) {
				boost::filesystem::remove_all(log_path);
			}
		}
};

TEST_F(CorrectnessTest, SystemLog) {
	boost::gregorian::date current_date(boost::gregorian::day_clock::local_day());

	ASSERT_EQ(0, LogToSystem(TRACE, "A system trace severity message %d", 666));
	ASSERT_EQ(0, LogToSystem(INFO, "A system INFO severity message %d", 667));
	ASSERT_EQ(0, LogToSystem(DEBUG, "A system DEBUG severity message %d", 668));
	ASSERT_EQ(0, LogToSystem(WARNING, "A system WARNING severity message %d", 669));
	ASSERT_EQ(0, LogToSystem(ERROR, "A system ERROR severity message %d", 670));

	std::stringstream stream;
	stream.imbue(std::locale(std::locale::classic(), new boost::gregorian::date_facet(ROTATE_FILE_NAME_FORMAT)));
	stream << current_date;
	std::string log_file_name = log_dir + "/" + system_log_name + stream.str() + boost::lexical_cast<std::string>(0);
	std::ifstream file(log_file_name);

	ASSERT_TRUE(file.good());
	int lines = std::count(std::istreambuf_iterator<char>(file), 
			             std::istreambuf_iterator<char>(), '\n');
	ASSERT_EQ(5, lines);
	file.close();
}

TEST_F(CorrectnessTest, DebugLog) {
	ASSERT_EQ(0, LogToDebug(TRACE, "A trace severity message %d", 666));
	ASSERT_EQ(0, LogToDebug(INFO, "A INFO severity message %d", 667));
	ASSERT_EQ(0, LogToDebug(DEBUG, "A DEBUG severity message %d", 668));
	ASSERT_EQ(0, LogToDebug(WARNING, "A WARNING severity message %d", 669));
	ASSERT_EQ(0, LogToDebug(ERROR, "A ERROR severity message %d", 670));

	std::ifstream file(log_dir + "/" + debug_log_name);
	ASSERT_TRUE(file.good());
	int lines = std::count(std::istreambuf_iterator<char>(file), 
			             std::istreambuf_iterator<char>(), '\n');
	ASSERT_EQ(5, lines);
	file.close();
}

TEST_F(CorrectnessTest, FileLog) {
	Logger log = SetLogName(log_file_name);
	ASSERT_TRUE(log);
	ASSERT_EQ(0, LogToFile(log, TRACE, "A trace severity message %s", log_file_name.c_str()));
	ASSERT_EQ(0, LogToFile(log, INFO, "A INFO severity message %s", log_file_name.c_str()));
	ASSERT_EQ(0, LogToFile(log, DEBUG, "A DEBUG severity message %s", log_file_name.c_str()));
	ASSERT_EQ(0, LogToFile(log, WARNING, "A WARNING severity message %s", log_file_name.c_str()));
	ASSERT_EQ(0, LogToFile(log, ERROR, "A ERROR severity message %s", log_file_name.c_str()));
	std::ifstream file(log_dir + "/" + log_file_name);
	ASSERT_TRUE(file.good());
	int lines = std::count(std::istreambuf_iterator<char>(file), 
			             std::istreambuf_iterator<char>(), '\n');
	ASSERT_EQ(5, lines);
	file.close();
}
void LogFunc(std::string file_name) {
	Logger log = SetLogName(file_name);
	ASSERT_TRUE(log);
	ASSERT_EQ(0, LogToFile(log, TRACE, "A trace severity message %s", log_file_name.c_str()));
	ASSERT_EQ(0, LogToFile(log, INFO, "A INFO severity message %s", log_file_name.c_str()));
	ASSERT_EQ(0, LogToFile(log, DEBUG, "A DEBUG severity message %s", log_file_name.c_str()));
	ASSERT_EQ(0, LogToFile(log, WARNING, "A WARNING severity message %s", log_file_name.c_str()));
	ASSERT_EQ(0, LogToFile(log, ERROR, "A ERROR severity message %s", log_file_name.c_str()));
}

TEST_F(CorrectnessTest, MultiThreadLog) {
	const int num_threads = 5;
	std::vector<std::thread> threads;

	for(int i = 0; i < num_threads; i++) {
		std::string tmp(1, '0'+i);
		tmp = log_file_prefix + tmp;
		threads.push_back(std::thread(LogFunc, tmp));
	}

	for(int i = 0; i < num_threads; i++) {
		threads[i].join();
	}

	for(int i = 0; i < num_threads; i++) {
		std::string tmp(1, '0'+i);
		tmp = log_dir + "/" + log_file_prefix + tmp;
		std::ifstream file(tmp);
		ASSERT_TRUE(file.good());
		int lines = std::count(std::istreambuf_iterator<char>(file), 
							 std::istreambuf_iterator<char>(), '\n');
		ASSERT_EQ(5, lines);
		file.close();
	}
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
