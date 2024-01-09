
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <string>


struct InfoForExtension
{
	const char* name;
	int64_t value;
};

struct SentenceInfo
{
	const InfoForExtension* infoArray;
	int64_t operator[](std::string propertyName)
	{
		for (auto info = infoArray; info->name; ++info) // nullptr name marks end of info array
			if (propertyName == info->name) return info->value;
		return *(int*)0xcccc = 0; // gives better error message than alternatives
	}
};

struct SKIP {};
inline void Skip() { throw SKIP(); }
