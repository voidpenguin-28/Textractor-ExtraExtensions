
#pragma once
#include "Common.h"

class GenderStrMapper {
public:
	virtual ~GenderStrMapper() { }
	virtual wstring map(Gender gender) const = 0;
	virtual Gender map(wstring gender) const = 0;
};


class DefaultGenderStrMapper : public GenderStrMapper {
public:
	wstring map(Gender gender) const {
		switch (gender) {
		case Gender::Male:
			return L"M";
		case Gender::Female:
			return L"F";
		default:
			return L"U";
		}
	}

	Gender map(wstring gender) const {
		if (gender == L"M")
			return Gender::Male;
		else if (gender == L"F")
			return Gender::Female;
		else
			return Gender::Unknown;
	}
};


class VndbHtmlGenderStrMapper : public GenderStrMapper {
public:
	wstring map(Gender gender) const {
		switch (gender) {
		case Gender::Male:
			return L"Male";
		case Gender::Female:
			return L"Female";
		default:
			return L"";
		}
	}

	Gender map(wstring gender) const {
		if (gender == L"Male")
			return Gender::Male;
		else if (gender == L"Female")
			return Gender::Female;
		else
			return Gender::Unknown;
	}
};