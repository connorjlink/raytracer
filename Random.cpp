import std;

#include "random.h"

namespace luma 
{
	static void Random::init()
	{
		_engine.seed(std::random_device()());
	}

	std::mt19937 Random::_engine;
	std::uniform_int_distribution<std::mt19937::result_type> Random::_distribution;
}