# slog
A simplify C++ Log Library based on Boost Log library.

## Interfaces
### Log Initialization API
```c
int InitLog(const std::string& subsystem_name, const std::string& log_path);
```
__Explain__

Initialize the log before logging to file.

__Parameters__

1. subsystem_name : set the system name.
2. log_path : the path where store the log files.

__Return Value__

0 : *success*.

-1 : *failed*.

### Log API
```c
int LogToSystem(slog::Severity severity, const char* format, ...);
```
__Explain__

Log the record to system log file (*default name: subsystem_name*).

__Parameters__

1. severity: log severity.
2. format: c printf-style input.

__Return Value__

0 : *success*.

-1 : *failed*.

## Usage
Just copy the __slogger.hpp__ to your project include directory.

## Requirements

|Libraries| Versions|
|-------- |--------:|
|Boost    | >= 1.57 |
|g++      | >= 4.4.7|
|cmake    | >= 2.8.12|

Comple Parametes: `-DBOOST_LOG_DYN_LINK -lboost_log -lboost_system -lboost_thread  -lboost_filesystem`

## Test
```bash
mkdir build
cd build
cmake ..

make

make test
```

## Sample
```c++
#include "slogger.hpp"

void TestLogSystem() {
	slog::LogToSystem(slog::TRACE, "A system trace severity message %d", 666);
	slog::LogToSystem(slog::INFO, "A system INFO severity message %d", 667);
	slog::LogToSystem(slog::DEBUG, "A system DEBUG severity message %d", 668);
	slog::LogToSystem(slog::WARNING, "A system WARNING severity message %d", 669);
	slog::LogToSystem(slog::ERROR, "A system ERROR severity message %d", 670);
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
	return 0;
}
```
