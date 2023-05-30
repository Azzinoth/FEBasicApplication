#include "FEUniqueID.h"
using namespace FocalEngine;

FEUniqueID* FEUniqueID::Instance = nullptr;

FEUniqueID::FEUniqueID()
{

}

std::string FEUniqueID::GetUniqueID()
{
	static std::random_device RandomDevice;
	static std::mt19937 mt(RandomDevice());
	static std::uniform_int_distribution<int> distribution(0, 128);

	static bool FirstInitialization = true;
	if (FirstInitialization)
	{
		srand(static_cast<unsigned>(time(nullptr)));
		FirstInitialization = false;
	}

	std::string ID;
	ID += static_cast<char>(distribution(mt));
	for (size_t j = 0; j < 11; j++)
	{
		ID.insert(rand() % ID.size(), 1, static_cast<char>(distribution(mt)));
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