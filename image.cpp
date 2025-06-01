import std;

#include "image.h"
#include "log.h"

// image.cpp
// (c) 2025 Connor J. Link. All Rights Reserved.

namespace luma
{
	Image::Image(std::int32_t width, std::int32_t height, bool allocate) noexcept
		: _width{ width }, _height{ height }
	{
		if (allocate)
		{
			_data = new std::uint32_t[width * height];
		}

		else
		{
			_data = nullptr;
		}
	}

	void Image::shadow_from(std::uint32_t* data) noexcept
	{
		_data = data;
	}

	void Image::export_to_ppm(const std::string& filepath) noexcept
	{
		// portable pixmap header
		auto ppm = std::format("P3\n{} {}\n255\n", _width, _height);

		for (auto x = 0; x < _width; x++)
		{
			for (auto y = 0; y < _height; y++)
			{
				const auto coordinate = y * _width + x;
				const auto data = _data[coordinate];

				const auto red = (data & 0xFF0000) >> 16;
				const auto green = (data & 0xFF00) >> 8;
				const auto blue = (data & 0xFF) >> 00;

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

	void Image::export_to_bmp(const std::string& filepath) noexcept
	{
		static constexpr int FILE_HEADER_SIZE = 14;
		static constexpr int INFO_HEADER_SIZE = 40;
		static constexpr int BYTES_PER_PIXEL = 3;

		// pad each row to a multiple of 4 bytes
		int row_stride = _width * BYTES_PER_PIXEL;
		int padding_size = (4 - (row_stride % 4)) % 4;
		int row_padded = row_stride + padding_size;
		int pixel_data_size = row_padded * _height;
		int file_size = FILE_HEADER_SIZE + INFO_HEADER_SIZE + pixel_data_size;

		unsigned char file_header[FILE_HEADER_SIZE] =
		{
			'B', 'M',   // magic
			0, 0, 0, 0, // filesize
			0, 0,       // reserved
			0, 0,       // reserved
			FILE_HEADER_SIZE + INFO_HEADER_SIZE, 0, 0, 0
		};

		unsigned char info_header[INFO_HEADER_SIZE] =
		{
			INFO_HEADER_SIZE, 0, 0, 0, // header size
			0, 0, 0, 0,                // image width
			0, 0, 0, 0,                // image height
			1, 0,                      // planes
			24, 0,                     // bits per pixel
			0, 0, 0, 0,                // compression level
			0, 0, 0, 0,                // image size (can be 0 for BI_RGB)
			0, 0, 0, 0,                // X pixels per meter
			0, 0, 0, 0,                // Y pixels per meter
			0, 0, 0, 0,                // total colors
			0, 0, 0, 0                 // important colors
		};

		// file size
		file_header[2] = (unsigned char)(file_size);
		file_header[3] = (unsigned char)(file_size >> 8);
		file_header[4] = (unsigned char)(file_size >> 16);
		file_header[5] = (unsigned char)(file_size >> 24);

		// width
		info_header[4] = (unsigned char)(_width);
		info_header[5] = (unsigned char)(_width >> 8);
		info_header[6] = (unsigned char)(_width >> 16);
		info_header[7] = (unsigned char)(_width >> 24);

		// height
		info_header[8] = (unsigned char)(_height);
		info_header[9] = (unsigned char)(_height >> 8);
		info_header[10] = (unsigned char)(_height >> 16);
		info_header[11] = (unsigned char)(_height >> 24);

		std::ofstream file(filepath, std::ios::out | std::ios::binary);
		if (!file)
		{
			log(std::format("error writing to file `{}`", filepath));
			return;
		}

		file.write(reinterpret_cast<char*>(file_header), FILE_HEADER_SIZE);
		file.write(reinterpret_cast<char*>(info_header), INFO_HEADER_SIZE);

		std::vector<unsigned char> padding(padding_size, 0);

		// store pixels bottom-up for BMP
		for (int y = _height - 1; y >= 0; --y)
		{
			for (int x = 0; x < _width; ++x)
			{
				const auto data = _data[y * _width + x];
				unsigned char blue = data & 0xFF;
				unsigned char green = (data >> 8) & 0xFF;
				unsigned char red = (data >> 16) & 0xFF;

				file.put(blue);
				file.put(green);
				file.put(red);
			}

			file.write(reinterpret_cast<char*>(padding.data()), padding_size);
		}

		delete[] _data;
		_data = nullptr;
	}
}
