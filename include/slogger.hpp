#ifndef LOG_LIBRARY_H_
#define LOG_LIBRARY_H_


#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

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
#include <cerrno>
#include <exception>

#include <fstream>
#include <string>
#include <memory>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#elif _LINUX
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace slog {


namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;
namespace filesystem = boost::filesystem;

enum SeverityLevel {
	TRACE, DEBUG, INFO, WARNING, ERROR, FATAL, UNDEFINED
};
// The formatting logic for the severity level
template<typename CharT, typename TraitsT>
inline std::basic_ostream<CharT, TraitsT>& operator<<(
		std::basic_ostream<CharT, TraitsT>& strm, SeverityLevel lvl) {
	static const char* const str[] = { "TRACE", "DEBUG", "INFO", "WARNING",
			"ERROR", "FATAL", "UNDEFINED" };
	if (static_cast<std::size_t>(lvl) < (sizeof(str) / sizeof(*str)))
		strm << str[lvl];
	else
		strm << static_cast<int>(lvl);
	return strm;
}

SeverityLevel StringToSeverity(const std::string& severity) {
	if( severity == "TRACE" ) {
		return TRACE;
	} else if( severity == "DEBUG" ) {
		return DEBUG;
	} else if( severity == "INFO" ) {
		return INFO;
	} else if( severity == "WARNING" ) {
		return WARNING;
	} else if( severity == "ERROR" ) {
		return ERROR;
	} else if( severity == "FATAL" ) {
		return FATAL;
	} else {
		return UNDEFINED;
	}
}

class UninitializedException: public std::exception {
	virtual const char* what() const throw(){
		return "Uninitialized log exception";
	}
} uninitializedException;

namespace log_inner {

const int BUFF_SIZE = 512;

const char* MODULE_FLAG_STRING = "Module";
const char* TYPE_FLAG_STRING = "Type";
const char* TIMESTAMP_FLAG_STRING = "TimeStamp";
const char* SCOPES_FLAG_STRING = "Scopes";
const char* SEVERITY_FLAG_STRING = "Severity";

const char* LOG_FILE_EXTENSION = ".log";
const char* FILE_ID_FLAG_STRING = "JobID";

class SLogger {
private:
	static SLogger* instance;
	static boost::mutex mu;
private:
	SLogger(const char* subsystem_name, const char* path,
			const char* sys_log_name, const char* debug_log_name) {
		InitPath(subsystem_name, path, sys_log_name, debug_log_name);
		InitLog();
		
	}
	~SLogger(){}
private:
	class Garbo {
		public:
			~Garbo(){
				if(SLogger::instance != NULL) {
					delete SLogger::instance;
				}
			}
	};
	static Garbo garbo;
public:
	static SLogger* GetInstance(const char* subsystem_name = "", const char* path = ".",
			const char* sys_log_name = NULL, const char* debug_log_name = NULL) {
		boost::mutex::scoped_lock lock(mu);
		if(instance == NULL) {
			instance = new SLogger(subsystem_name, path, sys_log_name, debug_log_name);
		}
		return instance;
	}

public:
	src::severity_logger_mt<SeverityLevel> debug_logger;

	src::severity_logger_mt<SeverityLevel> system_logger;

private:
	
	typedef sinks::synchronous_sink<sinks::text_multifile_backend> mb_sink_t;

	boost::shared_ptr<mb_sink_t> multiple_file_sink;

	std::string log_path;

	std::string debug_path;

	std::string system_path;

	std::string g_subsystem_name;

private:
	void InitLog() {
		//BOOST bugs for Boost::Filesystem
		filesystem::path::imbue(std::locale("C"));
		InitSources();
		InitGlobalAttributes();
		InitSinks();
	}

	bool CheckPathLegal(const char* path) {
		return filesystem::create_directories(path);
	}

	void InitPath(const char* subsystem_name, const char* path,
			const char* sys_log_name , const char* debug_log_name) {
		if(!CheckPathLegal(path)) {
			return;
		}
		debug_path = "_DEBUG";

		system_path = "";

		g_subsystem_name = "";

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

		boost::shared_ptr<text_file_sink_t> debug_text_sink(
				new text_file_sink_t(debug_text_backend));
		debug_text_sink->set_formatter(debug_formatter);
		debug_text_sink->set_filter(expr::attr<int>(TYPE_FLAG_STRING) == 0);
		core->add_sink(debug_text_sink);

		// init debug console sink
		BOOST_AUTO(console_sink, logging::add_console_log());
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
								> (FILE_ID_FLAG_STRING) ));

		// Wrap it into the frontend and register in the core.
		// The backend requires synchronization in the frontend.
		
		multiple_file_sink = boost::make_shared<mb_sink_t>(
				multiple_file_backend);

		// Set the formatter
		multiple_file_sink->set_formatter(system_formatter);
		multiple_file_sink->set_filter(expr::attr<int>(TYPE_FLAG_STRING) == 10);

		core->add_sink(multiple_file_sink);
	}

	void InitSources() {
		debug_logger.add_attribute(TYPE_FLAG_STRING, attrs::constant<int>(0));
		system_logger.add_attribute(TYPE_FLAG_STRING, attrs::constant<int>(1));
	}

};

//Initialization
SLogger* SLogger::instance = NULL;
boost::mutex SLogger::mu;
SLogger::Garbo SLogger::garbo;

SLogger* slogger = NULL;


}


// Logger API 
/*
 * Initialize the log library
 * @log_path : path of log files
 * @subsystem_name: name of module
 */
void InitLog(const std::string& subsystem_name, const std::string& log_path) {
	//BOOST bugs for Boost::Filesystem
	log_inner::slogger = log_inner::SLogger::GetInstance(subsystem_name.c_str(), log_path.c_str());
}

void InitLogWithSyslogName(const std::string& subsystem_name, const std::string& log_path,
		const std::string& sys_log_name) {
	log_inner::slogger = log_inner::SLogger::GetInstance(subsystem_name.c_str(), log_path.c_str(), sys_log_name.c_str());
}

void InitLogWithDebugLogName(const std::string& subsystem_name, const std::string& log_path,
		const std::string& debug_log_name) {
	log_inner::slogger = log_inner::SLogger::GetInstance(subsystem_name.c_str(), log_path.c_str(), NULL, debug_log_name.c_str());
}

void InitLogWithLogName(const std::string& subsystem_name, const std::string& log_path,
		const std::string& sys_log_name, const std::string& debug_log_name) {
	log_inner::slogger = log_inner::SLogger::GetInstance(subsystem_name.c_str(), log_path.c_str(), sys_log_name.c_str(), debug_log_name.c_str());
}
/*
 * Log api for debugging
 * @sl : severity level.
 * @message: log message.
 */
void LogToDebug(SeverityLevel sl, const char* format, ...) {
	if(log_inner::slogger == NULL) {
		throw uninitializedException;
	}
	char buffer[log_inner::BUFF_SIZE];
	char* pBuffer = buffer;
	va_list args;
	va_start(args, format);
	vsprintf(pBuffer, format, args);
	BOOST_LOG_SEV(log_inner::slogger->debug_logger, sl) << pBuffer;
}

/*
 * Log api for syslog
 * @sl : severity level.
 * @message: log message.
 */
void LogToSystem(SeverityLevel sl, const char* format, ...) {
	if(log_inner::slogger == NULL) {
		throw uninitializedException;
	}
	char buffer[log_inner::BUFF_SIZE];
	char* pBuffer = buffer;
	va_list args;
	va_start(args, format);
	vsprintf(pBuffer, format, args);
	BOOST_LOG_SEV(log_inner::slogger->system_logger, sl) << pBuffer;
}

typedef boost::shared_ptr<src::severity_logger_mt<SeverityLevel> > Logger;
/*
 * Log api for multiple file
 * @file_name: log file name
 *
 * @RETURN: logger object or null shared_ptr.
 */
Logger SetLogName(std::string file_name) {
	if(log_inner::slogger == NULL) {
		throw uninitializedException;
	}
	Logger multi_logger = boost::make_shared<
			src::severity_logger_mt<SeverityLevel> >();
	if(multi_logger != boost::shared_ptr<src::severity_logger_mt<SeverityLevel> >()) {
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
	if(log_inner::slogger == NULL) {
		throw uninitializedException;
	}
	char buffer[log_inner::BUFF_SIZE];
	char* pBuffer = buffer;
	va_list args;
	va_start(args, format);
	vsprintf(pBuffer, format, args);
	BOOST_LOG_SEV(*multi_logger, sl) << pBuffer;
}

}

#endif
