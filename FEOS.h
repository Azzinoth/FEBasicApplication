#pragma once

#include "FEBasicApplicationAPI.h"

namespace FocalEngine
{
	enum class OS
	{
		Unknown = 0,
		Windows = 1,
		MacOS = 2,
		Linux = 3,
		Android = 4,
		iOS = 5
	};

	FEBASICAPPLICATION_API OS GetOS();
}