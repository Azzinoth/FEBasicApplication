#include "FEUniqueID.h"
using namespace FocalEngine;

#ifdef FEBASICAPPLICATION_SHARED
extern "C" __declspec(dllexport) void* GetUniqueID()
{
	return FEUniqueID::GetInstancePointer();
}
#endif

FEUniqueID::FEUniqueID()
{

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