import std;

#include "log.h"

namespace luma
{
	void log(const std::string& msg)
	{
		std::println("[INFORMATION]: {}", msg);
	}
}
