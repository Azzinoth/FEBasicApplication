#pragma once

#include "../FEThreadPool.h"
#include "gtest/gtest.h"

#include <crtdbg.h>

class FEThreadPoolTest : public ::testing::Test
{
protected:
	void SetUp() override;
	void TearDown() override;

	// Pumps Update() until no thread has an active job, with a safety timeout.
	// Used to prevent state from one test leaking into the next.
	static void DrainThreadPool();
};

// Death-test children must never show interactive dialogs, otherwise the suite hangs.
inline void SuppressCrashDialogs()
{
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
}