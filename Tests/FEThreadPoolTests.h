#pragma once

#include "../FEThreadPool.h"
#include "gtest/gtest.h"

class FEThreadPoolTest : public ::testing::Test
{
protected:
	void SetUp() override;
	void TearDown() override;

	// Pumps Update() until no thread has an active job, with a safety timeout.
	// Used to prevent state from one test leaking into the next.
	static void DrainThreadPool();
};
