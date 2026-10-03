#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <time.h>
#include <random>
#include "FEBasicApplicationAPI.h"

#include "VersionInfo/FE_BASIC_APPLICATION_Version.h"
#include "VersionInfo/FEVersionInfo.h"

#include "ThirdParty/stduuid/uuid.h"

#define SINGLETON_PUBLIC_PART(CLASS_NAME)					\
    static CLASS_NAME& GetInstance()						\
    {														\
		/* Thread safe initialization*/						\
		static CLASS_NAME* Instance = new CLASS_NAME();		\
        return *Instance;									\
    }														\
															\
	static CLASS_NAME* GetInstancePointer()					\
	{														\
		return &GetInstance();								\
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
	using FEUUID = uuids::uuid;

	class FEBASICAPPLICATION_API FEUniqueID
	{
		SINGLETON_PRIVATE_PART(FEUniqueID)

		std::mt19937 RandomEngine;
		uuids::uuid_random_generator Generator;

		std::mutex IDGenerationMutex;
		std::string GetUniqueID();
	public:
		SINGLETON_PUBLIC_PART(FEUniqueID)

		FEUUID GetUUID();
		FEUUID GetNullUUID();
		bool IsNull(const FEUUID& ID);

		std::string ToString(const FEUUID& ID);
		FEUUID FromString(const std::string& ID);
		bool IsValid(const std::string& ID);

		FEUUID FromLegacyHexID(const std::string& HexID);
		// Accepts UUID strings and legacy hex IDs, empty string results in null UUID.
		FEUUID FromStringOrLegacyHexID(const std::string& ID);

		// This function can produce ID's that are "unique" with very rare collisions.
		// For most purposes it can be considered unique.
		// ID is a 24 long string.
		std::string GetUniqueHexID();
	};

#ifdef FEBASICAPPLICATION_SHARED
	extern "C" __declspec(dllexport) void* GetUniqueID();
	#define UNIQUE_ID (*static_cast<FEUniqueID*>(GetUniqueID()))
#else
	#define UNIQUE_ID FEUniqueID::GetInstance()
#endif
}