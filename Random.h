#ifndef LUMA_RANDOM_H
#define LUMA_RANDOM_H

#include <random>

namespace luma
{
	class Random
	{
	public:
		static void Init()
		{
			_engine.seed(std::random_device()());
		}

		static std::uint32_t uint()
		{
			return _distribution(_engine);
		}

		static std::uint32_t UInt(std::uint32_t min, std::uint32_t max)
		{
			return min + (_distribution(_engine) % (max - min + 1));
		}

		static float Float()
		{
			return static_cast<float>(_distribution(_engine)) / std::numeric_limits<uint32_t>::max();
		}

		static cjl::vec3 Vec3()
		{
			return cjl::vec3(Float(), Float(), Float());
		}

		static glm::vec3 Vec3(float min, float max)
		{
			return glm::vec3(Float() * (max - min) + min, Float() * (max - min) + min, Float() * (max - min) + min);
		}

		static cjl::vec3 InUnitSphere()
		{
			return cjl::normalize(Vec3(-1.0f, 1.0f));
		}

	private:
		static std::mt19937 _engine;
		static std::uniform_int_distribution<std::mt19937::result_type> _distribution;
	};

}

#endif
