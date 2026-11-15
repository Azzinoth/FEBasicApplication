#include "../SubSystems/FESystemInfo.h"
#include "gtest/gtest.h"

#include <memory>

using namespace FocalEngine;

TEST(FESystemInfoTest, ProcessMemoryUsage_RunningProcess_ReportsUsedAndPeak)
{
	const FEProcessMemoryUsage Memory = SYSTEM_INFORMATION.GetProcessMemoryUsage();

	EXPECT_GT(Memory.UsedBytes, 0u);
	EXPECT_GE(Memory.PeakUsedBytes, Memory.UsedBytes);
}

TEST(FESystemInfoTest, ProcessMemoryUsage_LargeAllocation_UsedGrowsAndPeakIsKept)
{
	const uint64_t BlockSize = 128ull * 1024 * 1024;
	const uint64_t ExpectedGrowth = BlockSize / 2;

	const FEProcessMemoryUsage Before = SYSTEM_INFORMATION.GetProcessMemoryUsage();

	std::unique_ptr<unsigned char[]> Block(new unsigned char[BlockSize]);
	// Memory is counted as resident only after it is written to.
	uint64_t Checksum = 0;
	for (uint64_t i = 0; i < BlockSize; i += 1024)
	{
		Block[i] = static_cast<unsigned char>(i >> 10);
		Checksum += Block[i];
	}
	EXPECT_GT(Checksum, 0u);

	const FEProcessMemoryUsage During = SYSTEM_INFORMATION.GetProcessMemoryUsage();
	EXPECT_GE(During.UsedBytes, Before.UsedBytes + ExpectedGrowth);
	EXPECT_GE(During.PeakUsedBytes, During.UsedBytes);

	Block.reset();

	const FEProcessMemoryUsage After = SYSTEM_INFORMATION.GetProcessMemoryUsage();
	EXPECT_GE(After.PeakUsedBytes, Before.UsedBytes + ExpectedGrowth);
}

TEST(FESystemInfoTest, ProcessMemoryUsageString_RunningProcess_ContainsUsedAndPeak)
{
	const std::string Text = SYSTEM_INFORMATION.GetProcessMemoryUsageString();

	EXPECT_NE(Text.find(" MB (peak "), std::string::npos) << Text;
}
