#include "FEUniqueID.h"
#include <algorithm>
using namespace FocalEngine;

#ifdef FEBASICAPPLICATION_SHARED
extern "C" __declspec(dllexport) void* GetUniqueID()
{
	return FEUniqueID::GetInstancePointer();
}
#endif

FEUniqueID::FEUniqueID() : Generator(RandomEngine)
{
	std::random_device RandomDevice;
	std::array<unsigned int, std::mt19937::state_size> SeedData;
	std::generate(SeedData.begin(), SeedData.end(), std::ref(RandomDevice));
	std::seed_seq Sequence(SeedData.begin(), SeedData.end());
	RandomEngine.seed(Sequence);
}

FEUUID FEUniqueID::GetUUID()
{
	std::lock_guard<std::mutex> Lock(IDGenerationMutex);
	return Generator();
}

FEUUID FEUniqueID::GetNullUUID()
{
	return FEUUID();
}

bool FEUniqueID::IsNull(const FEUUID& ID)
{
	return ID.is_nil();
}

std::string FEUniqueID::ToString(const FEUUID& ID)
{
	return uuids::to_string(ID);
}

FEUUID FEUniqueID::FromString(const std::string& ID)
{
	auto Result = uuids::uuid::from_string(ID);
	if (!Result.has_value())
		return GetNullUUID();
	
	return Result.value();
}

bool FEUniqueID::IsValid(const std::string& ID)
{
	return uuids::uuid::is_valid_uuid(ID);
}

FEUUID FEUniqueID::FromLegacyHexID(const std::string& HexID)
{
	// Fixed namespace for converting old hex IDs, must never change.
	static const FEUUID LegacyNamespace = uuids::uuid::from_string("040bcbf8-3a7c-4815-9da7-117bfb3f9bde").value();
	// Local instance because uuid_name_generator keeps hashing state and is not thread-safe.
	uuids::uuid_name_generator NameGenerator(LegacyNamespace);
	return NameGenerator(HexID);
}

FEUUID FEUniqueID::FromStringOrLegacyHexID(const std::string& ID)
{
	if (ID.empty())
		return GetNullUUID();

	if (IsValid(ID))
		return FromString(ID);

	return FromLegacyHexID(ID);
}

std::string FEUniqueID::GetUniqueID()
{
	static std::random_device RandomDevice;
	static std::mt19937 RandomEngine(RandomDevice());
	static std::uniform_int_distribution<int> Distribution(0, 128);

	static bool bIsFirstInitialization = true;
	if (bIsFirstInitialization)
	{
		srand(static_cast<unsigned>(time(nullptr)));
		bIsFirstInitialization = false;
	}

	std::string ID;
	ID += static_cast<char>(Distribution(RandomEngine));
	for (size_t j = 0; j < 11; j++)
	{
		ID.insert(rand() % ID.size(), 1, static_cast<char>(Distribution(RandomEngine)));
	}

	return ID;
}

std::string FEUniqueID::GetUniqueHexID()
{
	std::lock_guard<std::mutex> Lock(IDGenerationMutex);

	const std::string ID = GetUniqueID();
	std::string IDinHex;

	for (size_t i = 0; i < ID.size(); i++)
	{
		IDinHex.push_back("0123456789ABCDEF"[(ID[i] >> 4) & 15]);
		IDinHex.push_back("0123456789ABCDEF"[ID[i] & 15]);
	}

	const std::string AdditionalRandomness = GetUniqueID();
	std::string AdditionalString;
	for (size_t i = 0; i < ID.size(); i++)
	{
		AdditionalString.push_back("0123456789ABCDEF"[(AdditionalRandomness[i] >> 4) & 15]);
		AdditionalString.push_back("0123456789ABCDEF"[AdditionalRandomness[i] & 15]);
	}
	std::string FinalID;

	for (size_t i = 0; i < ID.size() * 2; i++)
	{
		if (rand() % 2 - 1)
		{
			FinalID += IDinHex[i];
		}
		else
		{
			FinalID += AdditionalString[i];
		}
	}

	return FinalID;
}