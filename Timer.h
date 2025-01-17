#ifndef LUMA_TIMER_H
#define LUMA_TIMER_H

namespace cjl
{
	class Timer
	{
	public:
		Timer() noexcept;

	public:
		void reset() noexcept;
	
	public:
		float seconds() const noexcept;
		float milliseconds() const noexcept;
		float nanoseconds() const noexcept;

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> _start;
	};

	class AutoTimer
	{
	public:
		AutoTimer(const std::string&) noexcept;
		~AutoTimer() noexcept;

	private:
		std::string _name;
		Timer _timer;
	};
}

#endif
