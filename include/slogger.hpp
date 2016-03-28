/*
 * log_test.cpp
 *
 *  Created on: 2016/3/7
 *      Author: Liu Qi
 */
#ifndef LOG_LIBRARY_H_
#define LOG_LIBRARY_H_

#include <sys/types.h>
#include <sys/stat.h>

#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>

#include <boost/log/core.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/trivial.hpp>

#include <cstdarg>
#include <cstdio>

#include <fstream>
#include <string>
#include <memory>

namespace slog {

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

enum SeverityLevel {
	TRACE, DEBUG, INFO, WARNING, ERROR, FATAL,
};
// The formatting logic for the severity level
template<typename CharT, typename TraitsT>
inline std::basic_ostream<CharT, TraitsT>& operator<<(
		std::basic_ostream<CharT, TraitsT>& strm, SeverityLevel lvl) {
	static const char* const str[] = { "TRACE", "DEBUG", "INFO", "WARNING",
			"ERROR", "FATAL" };
	if (static_cast<std::size_t>(lvl) < (sizeof(str) / sizeof(*str)))
		strm << str[lvl];
	else
		strm << static_cast<int>(lvl);
	return strm;
}

namespace log_inner {

const int BUFF_SIZE = 512;

const char* MODULE_FLAG_STRING = "Module";
const char* TYPE_FLAG_STRING = "Type";
const char* TIMESTAMP_FLAG_STRING = "TimeStamp";
const char* SCOPES_FLAG_STRING = "Scopes";
const char* SEVERITY_FLAG_STRING = "Severity";

const char* LOG_FILE_EXTENSION = ".log";

const char* FILE_ID_FLAG_STRING = "JobID";

src::severity_logger_mt<SeverityLevel> debug_logger;

src::severity_logger_mt<SeverityLevel> system_logger;

std::string log_path;

std::string debug_path = "_DEBUG";

std::string system_path = "";

std::string g_subsystem_name = "";

void InitPath(const char* subsystem_name, const char* path,
		const char* sys_log_name = NULL, const char* debug_log_name = NULL) {
	g_subsystem_name = std::string(subsystem_name);

	if (debug_log_name == NULL) {
		debug_path = g_subsystem_name + debug_path;
	} else {
		debug_path = debug_log_name;
	}
	if (sys_log_name == NULL) {
		system_path = g_subsystem_name;
	} else {
		system_path = sys_log_name;
	}
	log_path = std::string(path) + "/";
	debug_path = log_path + debug_path + LOG_FILE_EXTENSION;
	system_path = log_path + system_path + LOG_FILE_EXTENSION;

}

void InitGlobalAttributes() {
	logging::add_common_attributes();
	boost::shared_ptr < logging::core > core = logging::core::get();
	core->add_global_attribute(SCOPES_FLAG_STRING, attrs::named_scope());
	core->add_global_attribute(MODULE_FLAG_STRING,
			attrs::constant < std::string > (g_subsystem_name));
}

void InitSinks() {
	boost::shared_ptr < logging::core > core = logging::core::get();

	logging::formatter debug_formatter(
			expr::format("[%1%]<%2%> %3%  %4%: %5%") % expr::format_date_time
					< boost::posix_time::ptime
					> (TIMESTAMP_FLAG_STRING, "%Y-%m-%d %H:%M:%S") % expr::attr
					< std::string > (MODULE_FLAG_STRING) % expr::attr
					< SeverityLevel
					> (SEVERITY_FLAG_STRING)
							% expr::format_named_scope(SCOPES_FLAG_STRING,
									keywords::format = "%n %f:%l")
							% expr::smessage);
	//init debug file sink
	typedef sinks::synchronous_sink<sinks::text_file_backend> text_file_sink_t;

	boost::shared_ptr < sinks::text_file_backend > debug_text_backend =
			boost::make_shared < sinks::text_file_backend
					> (keywords::file_name = debug_path, keywords::auto_flush =
							true, keywords::open_mode = (std::ios::out
							| std::ios::app));

	debug_text_backend->auto_flush(true);
	boost::shared_ptr<text_file_sink_t> debug_text_sink(
			new text_file_sink_t(debug_text_backend));
	debug_text_sink->set_formatter(debug_formatter);
	debug_text_sink->set_filter(expr::attr<int>(TYPE_FLAG_STRING) == 0);
	core->add_sink(debug_text_sink);

	// init debug console sink
	auto console_sink = logging::add_console_log();
	console_sink->set_formatter(debug_formatter);
	console_sink->set_filter(
			expr::attr < SeverityLevel > (SEVERITY_FLAG_STRING) >= WARNING
					&& expr::attr<int>(TYPE_FLAG_STRING) == 0);

	//init sys file sink
	logging::formatter system_formatter(
			expr::format("[%1%]<%2%> %3%: %4%") % expr::format_date_time
					< boost::posix_time::ptime
					> (TIMESTAMP_FLAG_STRING, "%Y-%m-%d %H:%M:%S") % expr::attr
					< std::string > (MODULE_FLAG_STRING) % expr::attr
					< SeverityLevel > (SEVERITY_FLAG_STRING) % expr::smessage);

	boost::shared_ptr < sinks::text_file_backend > system_text_backend =
			boost::make_shared < sinks::text_file_backend
					> (keywords::file_name = system_path, keywords::auto_flush =
							true, keywords::open_mode = (std::ios::out
							| std::ios::app));

	boost::shared_ptr<text_file_sink_t> system_text_sink(
			new text_file_sink_t(system_text_backend));

	system_text_sink->set_formatter(system_formatter);
	system_text_sink->set_filter(expr::attr<int>(TYPE_FLAG_STRING) == 1);
	core->add_sink(system_text_sink);

	//init multiple files sink
	boost::shared_ptr < sinks::text_multifile_backend > multiple_file_backend =
			boost::make_shared<sinks::text_multifile_backend>();

	// Set up the file naming pattern
	multiple_file_backend->set_file_name_composer(
			sinks::file::as_file_name_composer(
					expr::stream << log_path << expr::attr < std::string
							> (FILE_ID_FLAG_STRING) << ".log"));

	// Wrap it into the frontend and register in the core.
	// The backend requires synchronization in the frontend.
	typedef sinks::synchronous_sink<sinks::text_multifile_backend> mb_sink_t;
	boost::shared_ptr<mb_sink_t> multiple_file_sink(
			new mb_sink_t(multiple_file_backend));

	// Set the formatter
	multiple_file_sink->set_formatter(system_formatter);
	multiple_file_sink->set_filter(expr::attr<int>(TYPE_FLAG_STRING) == 10);

	core->add_sink(multiple_file_sink);
}

void InitSources() {
	debug_logger.add_attribute(TYPE_FLAG_STRING, attrs::constant<int>(0));
	system_logger.add_attribute(TYPE_FLAG_STRING, attrs::constant<int>(1));
}

void InitLogInner() {
	//BOOST bugs for Boost::Filesystem
	boost::filesystem::path::imbue(std::locale("C"));
	InitSources();
	InitGlobalAttributes();
	InitSinks();
}

}
// Logger API 
/*
 * Initialize the log library
 * @log_path : path of log files
 * @subsystem_name: name of module
 */
void InitLog(const char* subsystem_name, const char* log_path) {
	//BOOST bugs for Boost::Filesystem
	log_inner::InitPath(subsystem_name, log_path);
	log_inner::InitLogInner();
}

void InitLogWithSyslogName(const char* subsystem_name, const char* log_path,
		const char* sys_log_name) {
	log_inner::InitPath(subsystem_name, log_path, sys_log_name);
	log_inner::InitLogInner();
}

void InitLogWithDebugLogName(const char* subsystem_name, const char* log_path,
		const char* debug_log_name) {
	log_inner::InitPath(subsystem_name, log_path, NULL, debug_log_name);
	log_inner::InitLogInner();
}

void InitLogWithLogName(const char* subsystem_name, const char* log_path,
		const char* sys_log_name, const char* debug_log_name) {
	log_inner::InitPath(subsystem_name, log_path, sys_log_name, debug_log_name);
	log_inner::InitLogInner();
}
/*
 * Log api for debugging
 * @sl : severity level.
 * @message: log message.
 */
void LogToDebug(SeverityLevel sl, const char* format, ...) {
	char buffer[log_inner::BUFF_SIZE];
	char* pBuffer = buffer;
	va_list args;
	va_start(args, format);
	vsprintf(pBuffer, format, args);
	BOOST_LOG_SEV(log_inner::debug_logger, sl) << pBuffer;
}

/*
 * Log api for syslog
 * @sl : severity level.
 * @message: log message.
 */
void LogToSystem(SeverityLevel sl, const char* format, ...) {
	char buffer[log_inner::BUFF_SIZE];
	char* pBuffer = buffer;
	va_list args;
	va_start(args, format);
	vsprintf(pBuffer, format, args);
	BOOST_LOG_SEV(log_inner::system_logger, sl) << pBuffer;
}

typedef std::shared_ptr<src::severity_logger_mt<SeverityLevel>> Logger;
/*
 * Log api for multiple file
 * @file_name: log file name
 *
 * @RETURN: logger object or null shared_ptr.
 */
Logger SetLogName(std::string file_name) {
	Logger multi_logger = std::make_shared<
			src::severity_logger_mt<SeverityLevel> >();
	if (!multi_logger) {
		multi_logger->add_attribute(log_inner::FILE_ID_FLAG_STRING,
				attrs::constant < std::string > (file_name));
		multi_logger->add_attribute(log_inner::TYPE_FLAG_STRING,
				attrs::constant<int>(10));
	}
	return multi_logger;
}

/*
 * Log api for writing specified log file (multiple thread)
 * @logger: log object
 * @sl : severity level.
 * @format: format string.
 * @variadic params
 */
void LogToFile(Logger multi_logger, SeverityLevel sl, const char* format, ...) {
	char buffer[log_inner::BUFF_SIZE];
	char* pBuffer = buffer;
	va_list args;
	va_start(args, format);
	vsprintf(pBuffer, format, args);
	BOOST_LOG_SEV(*multi_logger, sl) << pBuffer;
}

/*
 * Log api for writing specified log file (multiple thread)
 * @file_name: log file name
 * @sl : severity level.
 * @format: format string.
 * @variadic params
 */

void Log(std::string file_name, SeverityLevel sl, const char* format, ...) {
	src::severity_logger_mt<SeverityLevel> logger;
	logger.add_attribute(log_inner::FILE_ID_FLAG_STRING,
			attrs::constant < std::string > (file_name));
	logger.add_attribute(log_inner::TYPE_FLAG_STRING, attrs::constant<int>(10));
	char buffer[log_inner::BUFF_SIZE];
	char* pBuffer = buffer;
	va_list args;
	va_start(args, format);
	vsprintf(pBuffer, format, args);
	BOOST_LOG_SEV(logger, sl) << pBuffer;
}

}

#endif
