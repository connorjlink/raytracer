import std;

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "arguments.h"
#include "application.h"
#include "image.h"
#include "renderer.h"

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

		//std::this_thread::sleep_for(200ms);

		return true;
	}
};

int main(int argc, char** argv)
{
	auto arguments = luma::Arguments{};
	arguments.parse(argc, argv);

	Luma demo{};
	if (demo.Construct(200, 200, 5, 5))
		demo.Start();

	return 0;
}
