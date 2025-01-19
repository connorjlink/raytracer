import std;

#include "timer.h"

namespace cjl
{
	Timer::Timer() noexcept
	{
		reset();
	}

	void Timer::reset() noexcept
	{
		_start = std::chrono::high_resolution_clock::now();
	}

	float Timer::seconds() const noexcept
	{
		const auto now = std::chrono::high_resolution_clock::now();
		const auto diff = now - _start;
		const auto result = std::chrono::duration_cast<std::chrono::seconds>(diff).count();
		return static_cast<float>(result);
	}

	float Timer::milliseconds() const noexcept
	{
		const auto now = std::chrono::high_resolution_clock::now();
		const auto diff = now - _start;
		const auto result = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
		return static_cast<float>(result);
	}

	float Timer::nanoseconds() const noexcept
	{
		const auto now = std::chrono::high_resolution_clock::now();
		const auto diff = now - _start;
		const auto result = std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count();
		return static_cast<float>(result);
	}


	AutoTimer::AutoTimer(const std::string& name) noexcept
		: _name{ name }, _timer{}
	{
	}

	AutoTimer::~AutoTimer()
	{
		float time = _timer.milliseconds();
		std::println("[TIMER] {} - {}ms", _name, time);
	}
}