#include "../SubSystems/FESystemInfo.h"
#include "../FEPlatform.h"
#include "gtest/gtest.h"

#include <atomic>
#include <memory>
#include <thread>

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
	// Volatile, otherwise optimizer removes the writes together with the allocation.
	volatile unsigned char* BlockData = Block.get();
	for (uint64_t i = 0; i < BlockSize; i += 1024)
		BlockData[i] = static_cast<unsigned char>(i >> 10);

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

TEST(FESystemInfoTest, SystemLoad_ShortMeasurement_CPULoadIsInRange)
{
	const FESystemLoad Load = SYSTEM_INFORMATION.GetSystemLoad(0.2);

	ASSERT_TRUE(Load.bCPULoadAvailable);
	EXPECT_GE(Load.CPULoadPercentage, 0.0);
	EXPECT_LE(Load.CPULoadPercentage, 100.0);
}

TEST(FESystemInfoTest, SystemLoad_AllCoresBusy_CPULoadIsHigh)
{
	std::atomic<bool> bStop = false;
	std::vector<std::thread> Threads;
	const int ThreadCount = SYSTEM_INFORMATION.GetCPUInfo().LogicalCoreCount;
	for (int i = 0; i < ThreadCount; i++)
		Threads.emplace_back([&bStop]() { while (!bStop) {} });

	const FESystemLoad Load = SYSTEM_INFORMATION.GetSystemLoad(0.5);

	bStop = true;
	for (std::thread& Thread : Threads)
		Thread.join();

	ASSERT_TRUE(Load.bCPULoadAvailable);
	// Far above load of a machine that only runs tests, and below 100 because a shared machine does not give all of its time.
	EXPECT_GE(Load.CPULoadPercentage, 50.0);
}

TEST(FESystemInfoTest, SystemLoad_GPULoad_IsReportedOnlyOnWindows)
{
	const FESystemLoad Load = SYSTEM_INFORMATION.GetSystemLoad(0.2);

#if !FE_PLATFORM(WINDOWS)
	EXPECT_FALSE(Load.bGPULoadAvailable);
#endif

	// A Windows machine without a GPU driver, like a virtual machine, does not report GPU load either.
	if (!Load.bGPULoadAvailable)
	{
		EXPECT_EQ(Load.GPULoadPercentage, 0.0);
		EXPECT_TRUE(Load.GPULoadByProcess.empty());
		return;
	}

	EXPECT_GE(Load.GPULoadPercentage, 0.0);
	EXPECT_LE(Load.GPULoadPercentage, 100.0);
	for (size_t i = 0; i < Load.GPULoadByProcess.size(); i++)
	{
		EXPECT_GT(Load.GPULoadByProcess[i].Percentage, 0.0);
		EXPECT_LE(Load.GPULoadByProcess[i].Percentage, 100.0);
		EXPECT_NE(Load.GPULoadByProcess[i].ProcessID, 0u);
		if (i > 0)
			EXPECT_LE(Load.GPULoadByProcess[i].Percentage, Load.GPULoadByProcess[i - 1].Percentage);
	}
}

TEST(FESystemInfoTest, SystemLoadString_ShortMeasurement_HasCPUAndGPUParts)
{
	const std::string Text = SYSTEM_INFORMATION.GetSystemLoadString(0.2);

	EXPECT_EQ(Text.find("CPU "), 0u) << Text;
	EXPECT_NE(Text.find(", GPU "), std::string::npos) << Text;
}
