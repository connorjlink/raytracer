#ifndef LUMA_ARGUMENTS_H
#define LUMA_ARGUMENTS_H

namespace luma
{
	enum class RenderMode
	{
		RAYTRACE,
		PATHTRACE,
	};

	struct Options
	{
		std::uint32_t width, height;
		std::uint32_t samples, bounces;
		RenderMode mode;
	};

	inline static Options _options{};

	class Arguments
	{
	private:
		std::vector<std::string> _filepaths;

	public:
		const decltype(_filepaths)& files() const
		{
			return _filepaths;
		}

	public:
		Arguments()
		{
		}

	public:
		void parse(int, char**);
	};
}

#endif