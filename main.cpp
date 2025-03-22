import std;

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "arguments.h"
#include "application.h"
#include "image.h"
#include "renderer.h"

// main.cpp
// (c) 2025 Connor J. Link. All Rights Reserved.

luma::Options luma::_options;

using namespace std::chrono_literals;

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

		// set up some sensible defaults in case the command line arguments are missing or incomplete
		/*luma::_options.width = ScreenWidth();
		luma::_options.height = ScreenHeight();
		luma::_options.bounces = 2;
		luma::_options.samples = 2;
		luma::_options.paths = 1;
		luma::_options.mode = luma::RenderMode::PATHTRACE;*/

		return true;
	}

	bool OnUserUpdate(float Δt) override
	{
		renderer.render_to(framebuffer, *this);

		if (GetKey(olc::Key::CTRL).bHeld &&
			GetKey(olc::Key::P).bHeld)
		{
			image.shadow_from(framebuffer);
			image.export_to("luma.ppm");

			std::println("successfully exported frame capture to `luma.ppm`");
		}

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

		const auto& dir = renderer.camera.dir;
		const auto& right = renderer.camera.right;

		/*DrawStringDecal({ 1.f, 1.f }, std::format("Forward: ({}, {}, {})", dir[0], dir[1], dir[2]));
		DrawStringDecal({ 1.f, 11.f }, std::format("Right: ({}, {}, {})", right[0], right[1], right[2]));
		DrawStringDecal({ 1.f, 21.f }, std::format("Depth: {}", renderer.camera.depth));*/

		//std::this_thread::sleep_for(200ms);

		return true;
	}
};

int main(int argc, char** argv)
{
	auto arguments = luma::Arguments{};
	arguments.parse(argc, argv);

	Luma demo{};
	if (demo.Construct(1584, 396, 1, 1, false, false, true, true))
		demo.Start();

	return 0;
}
