#ifndef LUMA_ARGUMENTS_H
#define LUMA_ARGUMENTS_H

// arguments.h
// (c) 2025 Connor J. Link. All Rights Reserved.

namespace luma
{
	enum class RenderMode
	{
		RAYTRACE,
		PATHTRACE,
	};

	static const std::unordered_map<std::string, RenderMode> _render_mode_map
	{
		{ "raytrace", RenderMode::RAYTRACE },
		{ "pathtrace", RenderMode::PATHTRACE },
	};

	enum class Context
	{
		INTERACTIVE,
		HEADLESS,
	};

	static const std::unordered_map<std::string, Context> _context_map
	{
		{ "interative", Context::INTERACTIVE },
		{ "headless", Context::HEADLESS },
	};

	struct Options
	{
		std::uint32_t width, height;
		std::uint32_t samples, bounces, paths;
		RenderMode mode;
		Context context = Context::INTERACTIVE;
	};

	extern Options _options;

	class Arguments
	{
	public:
		void parse(int, char**);
	};
}

#endif