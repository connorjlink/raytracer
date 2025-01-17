#ifndef LUMA_ARGUMENTS_H
#define LUMA_ARGUMENTS_H

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

	struct Options
	{
		std::uint32_t width, height;
		std::uint32_t samples, bounces;
		RenderMode mode;
	};

	extern Options _options;

	class Arguments
	{
	private:
	public:
		void parse(int, char**);
	};
}

#endif