#include "gtest/gtest.h"

int main(int ArgumentCount, char** Arguments)
{
	testing::GTEST_FLAG(output) = "xml:FEBasicApplicationTests.xml";
	testing::InitGoogleTest(&ArgumentCount, Arguments);

	return RUN_ALL_TESTS();
}
