#include "random.h"

namespace luma 
{
	std::mt19937 Random::_engine;
	std::uniform_int_distribution<std::mt19937::result_type> Random::_distribution;
}