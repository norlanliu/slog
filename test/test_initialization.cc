#include "slogger.hpp"
#include "gtest/gtest.h"

TEST(Initialization, Uninitialized) {
	ASSERT_EQ(slog::LogToSystem(slog::ERROR, "hehe"), -1);
	ASSERT_EQ(slog::LogToDebug(slog::ERROR, "hehe"), -1);
	ASSERT_FALSE(slog::SetLogName("hehe"));
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
