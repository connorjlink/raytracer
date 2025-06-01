import std;

#include "log.h"

// log.cpp
// (c) 2025 Connor J. Link. All Rights Reserved.

namespace luma
{
	void log(const std::string& msg)
	{
		std::println("[INFORMATION]: {}", msg);
	}

	void warning(const std::string& msg)
	{
		std::println("[WARNING]: {}", msg);
	}
}
