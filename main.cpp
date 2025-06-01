import std;

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "arguments.h"
#include "application.h"
#include "image.h"
#include "renderer.h"

luma::Options luma::_options;

using namespace std::chrono_literals;

namespace
{
	static constexpr auto SHARED_MEMORY_ID = L"LumaFramebuffer";

	void* pBuf = nullptr;

	void WriteFramebuffer(const uint8_t* data, size_t size, HANDLE file_map)
	{
		if (pBuf)
		{
			memcpy(pBuf, data, size);
		}
	}
}

class Luma : public olc::PixelGameEngine
{
public:
	Luma()
		: image{ ScreenWidth(), ScreenHeight(), false }
	{
		window_title = "Luma";
	}

private:
	luma::Application application;
	luma::Renderer renderer;
	luma::Image image;
	std::uint32_t* framebuffer;

public:
	bool OnUserCreate() override
	{
		application = {};
		renderer = {};

		framebuffer = new std::uint32_t[ScreenWidth() * ScreenHeight()];

		return true;
	}

	bool OnUserUpdate(float timestep) override
	{
		renderer.render_to(framebuffer, this);

		for (auto x = 0; x < ScreenWidth(); x++)
		{
			for (auto y = 0; y < ScreenHeight(); y++)
			{
				const auto data = framebuffer[y * ScreenWidth() + x];

				const std::uint8_t red = (data & 0xFF0000) >> 16;
				const std::uint8_t green = (data & 0xFF00) >> 8;
				const std::uint8_t blue = (data & 0xFF) >> 0;

				const olc::Pixel pixel{ red, green, blue };
				Draw(x, y, pixel);
			}
		}

		if (GetKey(olc::Key::CTRL).bHeld)
		{
			if (GetKey(olc::Key::P).bPressed)
			{
				image.shadow_from(framebuffer);
				image.export_to_ppm("luma.ppm");
				std::println("successfully exported frame capture to `luma.ppm`");
			}

			else if (GetKey(olc::Key::B).bPressed)
			{
				image.shadow_from(framebuffer);
				image.export_to_ppm("luma.bmp");
				std::println("successfully exported frame capture to `luma.bmp`");
			}
		}

		//std::this_thread::sleep_for(200ms);

		return true;
	}
};

int main(int argc, char** argv)
{
	auto arguments = luma::Arguments{};
	arguments.parse(argc, argv);

	using enum luma::Context;
	switch (luma::_options.context)
	{
		case INTERACTIVE:
		{
			Luma demo{};
			if (demo.Construct(500, 500, 2, 2))
			{
				demo.Start();
			}
		} break;

		case HEADLESS:
		{
			luma::Renderer renderer{};

			const auto pixel_count = luma::_options.width * luma::_options.height;
			// 32 bits per pixel
			const auto bytes = pixel_count * 4;
			
			auto file_map = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, static_cast<DWORD>(bytes), SHARED_MEMORY_ID);
			if (!file_map)
			{
				std::println("failed to create the memory-mapped resource: {}", GetLastError());
				break;
			}

			pBuf = MapViewOfFile(file_map, FILE_MAP_WRITE, 0, 0, bytes);


			std::uint32_t* framebuffer = new std::uint32_t[pixel_count];

			auto then = std::chrono::high_resolution_clock::now();

			auto frame_count = 0;

			while (true)
			{
				renderer.render_to(framebuffer, nullptr);

				const auto now = std::chrono::high_resolution_clock::now();
				const auto delta = now - then;
				then = now;
				const auto frametime = std::chrono::duration_cast<std::chrono::milliseconds>(delta);

				::WriteFramebuffer(reinterpret_cast<uint8_t*>(framebuffer), bytes, file_map);

				std::println("[DELIVERED FRAME]:{}:{}", frame_count, frametime.count());
				frame_count++;
			}

			if (pBuf)
			{
				UnmapViewOfFile(pBuf);
			}

			CloseHandle(file_map);

		} break;

		default:
			break;
	}

	return 0;
}
