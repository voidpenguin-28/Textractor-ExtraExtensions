#pragma once

#include <string>
#include <unordered_map>
using namespace std;

enum class Gender { Unknown = 0, Male, Female };

template<typename T> using templ_map = unordered_map<wstring, T>;
typedef templ_map<wstring> wstring_map;
typedef templ_map<Gender> gender_map;


struct CharMappings {
	wstring_map fullNameMap;
	wstring_map singleNameMap;
	gender_map genderMap;

	CharMappings(wstring_map fullNameMap_ = {}, wstring_map singleNameMap_ = {}, gender_map genderMap_ = {})
		: fullNameMap(fullNameMap_), singleNameMap(singleNameMap_), genderMap(genderMap_) { }
};
