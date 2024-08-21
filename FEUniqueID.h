#pragma once

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include <string>
#include <unordered_map>
#include <chrono>
#include <time.h>
#include <random>
#include "FEBasicApplicationAPI.h"

#define SINGLETON_PUBLIC_PART(CLASS_NAME)		\
    static CLASS_NAME& GetInstance()			\
    {											\
		static CLASS_NAME* Instance = nullptr;	\
        if (!Instance)							\
            Instance = new CLASS_NAME();		\
        return *Instance;						\
    }											\
												\
	static CLASS_NAME* GetInstancePointer()		\
	{											\
		return &GetInstance();					\
	}

#define SINGLETON_PRIVATE_PART(CLASS_NAME)	\
    CLASS_NAME();							\
	~CLASS_NAME();							\
    CLASS_NAME(const CLASS_NAME &);			\
    void operator= (const CLASS_NAME &);


#define FE_MAP_TO_STR_VECTOR(map)          \
	std::vector<std::string> result;       \
	auto iterator = map.begin();           \
	while (iterator != map.end())          \
	{                                      \
		result.push_back(iterator->first); \
		iterator++;                        \
	}                                      \
										   \
	return result;

namespace FocalEngine
{
	class FEBASICAPPLICATION_API FEUniqueID
	{
		SINGLETON_PRIVATE_PART(FEUniqueID)

		std::string GetUniqueID();
	public:
		SINGLETON_PUBLIC_PART(FEUniqueID)

		// This function can produce ID's that are "unique" with very rare collisions.
		// For most purposes it can be considered unique.
		// ID is a 24 long string.
		std::string GetUniqueHexID();
	};

#define UNIQUE_ID FEUniqueID::GetInstance()
}