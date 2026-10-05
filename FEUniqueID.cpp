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

FEUUID FEUniqueID::GenerateID()
{
	std::lock_guard<std::mutex> Lock(IDGenerationMutex);
	return Generator();
}

FEUUID FEUniqueID::ConvertLegacyHexID(const std::string& HexID)
{
	// Fixed namespace for converting old hex IDs, must never change.
	static const FEUUID LegacyIDNamespace = uuids::uuid::from_string("040bcbf8-3a7c-4815-9da7-117bfb3f9bde").value();
	uuids::uuid_name_generator NameGenerator(LegacyIDNamespace);
	return NameGenerator(HexID);
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
		return FEUUID();

	return Result.value();
}