#ifndef LUMA_RANDOM_H
#define LUMA_RANDOM_H

#include "base.h"
#include "types.h"

namespace luma
{
	class Random
	{
	public:
		static void init();

		static std::uint32_t uint();
		static std::uint32_t uint(std::uint32_t min, std::uint32_t max);

		static float real();

		static cjl::vec3 vec3();
		static cjl::vec3 vec3(float, float);
		static cjl::vec3 vec3_onsphere();

	private:
		static std::mt19937 _engine;
		static std::uniform_int_distribution<std::mt19937::result_type> _distribution;
	};

}

#endif
