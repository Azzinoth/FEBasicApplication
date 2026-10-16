#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
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
	std::vector<std::string> Result;       \
	auto Iterator = map.begin();           \
	while (Iterator != map.end())          \
	{                                      \
		Result.push_back(Iterator->first); \
		Iterator++;                        \
	}                                      \
										   \
	return Result;

namespace FocalEngine
{
	using FEUUID = uuids::uuid;

	class FEBASICAPPLICATION_API FEUniqueID
	{
		SINGLETON_PRIVATE_PART(FEUniqueID)

		std::mt19937 RandomEngine;
		uuids::uuid_random_generator Generator;

		std::mutex IDGenerationMutex;
	public:
		SINGLETON_PUBLIC_PART(FEUniqueID)

		FEUUID GenerateID();
		FEUUID ConvertLegacyHexID(const std::string& HexID);
		bool IsNull(const FEUUID& ID);

		std::string ToString(const FEUUID& ID);
		FEUUID FromString(const std::string& ID);
		FEUUID FromStringLegacyCompatible(const std::string& ID);
	};

#ifdef FEBASICAPPLICATION_SHARED
	extern "C" __declspec(dllexport) void* GetUniqueID();
	#define UNIQUE_ID (*static_cast<FEUniqueID*>(GetUniqueID()))
#else
	#define UNIQUE_ID FEUniqueID::GetInstance()
#endif
}