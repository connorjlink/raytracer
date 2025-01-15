#ifndef LUMA_TIMER_H
#define LUMA_TIMER_H

#include <iostream>
#include <string>
#include <chrono>

namespace cjl
{
	class Timer
	{
	public:
		Timer()
		{
			reset();
		}

	public:
		void reset()
		{
			_start = std::chrono::high_resolution_clock::now();
		}

	private:
		auto _delta()
		{
			const auto now = std::chrono::high_resolution_clock::now();
			const auto diff = now - _start;
			return diff;
		}

	public:
		float seconds()
		{
			const auto diff = _delta();
			const auto result = std::chrono::duration_cast<std::chrono::seconds>(diff).count();
			return result;
		}

		float milliseconds()
		{
			const auto diff = _delta();
			const auto result = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
			return result;
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> _start;
	};

	class AutoTimer
	{
	public:
		AutoTimer(const std::string& name)
			: _name(name) 
		{
			_timer{};
		}

		~AutoTimer()
		{
			float time = _timer.milliseconds();
			std::println("[TIMER] {} - {}ms");
		}
	private:
		std::string _name;
		Timer _timer;
	};
}

#endif
