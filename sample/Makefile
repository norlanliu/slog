TARGET = main
OBJS = log_test.cc
INC_DIR = -I./include
CXXFLAGS = -g -std=c++0x -Wall -DBOOST_LOG_DYN_LINK -lboost_log -lboost_system -lboost_thread -lboost_filesystem
CXX = g++


$(TARGET) : $(OBJS)
	$(CXX) $(INC_DIR) $(CXXFLAGS) -o $@ $^

.PHONY : clean

clean:
	rm *.o $(TARGET)
