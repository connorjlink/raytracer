import std;

#include "image.h"
#include "log.h"

namespace luma
{
	Image::Image(std::uint32_t width, std::uint32_t height) noexcept
		: _width{ width }, _height{ height }, _data{ nullptr }
	{
	}

	void Image::shadow_from(std::uint32_t* data) noexcept
	{
		_data = data;
	}

	void Image::export_to(const std::string& filepath) noexcept
	{
		// portable pixmap header
		auto ppm = std::format("P3\n{} {}\n255\n", _width, _height);

		for (auto x = 0; x < _width; x++)
		{
			for (auto y = 0; y < _height; y++)
			{
				const auto coordinate = y * _width + x;
				const auto data = _data[coordinate];

				const auto red = data & 0xFF0000 >> 16;
				const auto green = data & 0xFF00 >> 8;
				const auto blue = data & 0xFF >> 00;

				ppm += std::format("{} {} {}  ", red, green, blue);
			}

			ppm += '\n';
		}

		if (std::ofstream file(filepath); file.good())
		{
			file.write(ppm.data(), ppm.size());

			delete[] _data;
			_data = nullptr;
		}

		else
		{
			log(std::format("error writing to file `{}`", filepath));
		}
	}

}
