#pragma once
#include <chrono>
#include <ctime>
#include <string>

inline std::string getCurrentDateTime() {
	auto currentTime = std::chrono::system_clock::now();
	time_t currentTime_t = std::chrono::system_clock::to_time_t(currentTime);

	struct tm currentTime_tm;
	localtime_s(&currentTime_tm, &currentTime_t);

	char buffer[80];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &currentTime_tm);
	return std::string(buffer);
}
