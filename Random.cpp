import std;

#include "random.h"
#include "vector.h"

namespace luma 
{
	void Random::init()
	{
		_engine.seed(std::random_device()());
	}

	std::uint32_t Random::uint()
	{
		return _distribution(_engine);
	}

	std::uint32_t Random::uint(std::uint32_t min, std::uint32_t max)
	{
		return min + (_distribution(_engine) % (max - min + 1));
	}

	float Random::real()
	{
		return static_cast<float>(_distribution(_engine)) / std::numeric_limits<std::uint32_t>::max();
	}

	cjl::vec3 Random::vec3()
	{
		return cjl::vec3{ real(), real(), real() };
	}

	cjl::vec3 Random::vec3(float min, float max)
	{
		auto clip = [&](auto value)
		{
			return value * (max - min) + min;
		};

		return cjl::vec3{ clip(real()), clip(real()), clip(real()) };
	}

	cjl::vec3 Random::vec3_onsphere()
	{
		return cjl::normalize(vec3(-1.0f, 1.0f));
	}

	std::mt19937 Random::_engine;
	std::uniform_int_distribution<std::mt19937::result_type> Random::_distribution;
}