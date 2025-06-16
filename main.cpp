#include <cstdint>
#include <chrono>
#include <functional>
#include <map>
#include <print>
#include <filesystem>
#include <array>
#include <vector>
#include <iostream>
#include <fstream>
#include <thread>

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "arguments.h"
#include "image.h"
#include "renderer.h"
#include "gpu.h"

#include <windows.h>

// main.cpp
// (c) 2025 Connor J. Link. All Rights Reserved.

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
	luma::Renderer renderer;
	luma::Image image;
	std::uint32_t* framebuffer;

public:
	bool OnUserCreate() override
	{
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

	bool OnUserUpdate(float timestep) override
	{
		renderer.render_to(framebuffer, this);

		/*for (auto x = 0; x < ScreenWidth(); x++)
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
		}*/

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
		
		const auto& dir = renderer.camera.dir;
		const auto& right = renderer.camera.right;

		/*DrawStringDecal({ 1.f, 1.f }, std::format("Forward: ({}, {}, {})", dir[0], dir[1], dir[2]));
		DrawStringDecal({ 1.f, 11.f }, std::format("Right: ({}, {}, {})", right[0], right[1], right[2]));
		DrawStringDecal({ 1.f, 21.f }, std::format("Depth: {}", renderer.camera.depth));*/

		//std::this_thread::sleep_for(200ms);

		return true;
	}
};

inline std::string LPWSTR_to_string(LPWSTR wstr)
{
	if (!wstr) 
		return {};

	int size_needed = WideCharToMultiByte(
		CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);

	if (size_needed <= 0)
		return {};

	std::string result(size_needed - 1, 0);

	WideCharToMultiByte(
		CP_UTF8, 0, wstr, -1, result.data(), size_needed, nullptr, nullptr);

	return result;
}

inline std::vector<std::string> ConvertArgvWToArgvA(LPWSTR* argvw, int argc)
{
	std::vector<std::string> result{};
	result.reserve(argc);

	for (int i = 0; i < argc; ++i)
	{
		int size_needed = WideCharToMultiByte(
			CP_UTF8, 0, argvw[i], -1, nullptr, 0, nullptr, nullptr);

		if (size_needed > 0)
		{
			std::string arg(size_needed - 1, 0);

			WideCharToMultiByte(
				CP_UTF8, 0, argvw[i], -1, arg.data(), size_needed, nullptr, nullptr);

			result.push_back(std::move(arg));
		}

		else
		{
			result.push_back({});
		}
	}

	return result;
}

inline std::vector<char*> MakeArgv(const std::vector<std::string>& args)
{
	std::vector<char*> argv{};
	argv.reserve(args.size() + 1);

	for (const auto& s : args)
		argv.push_back(const_cast<char*>(s.c_str()));

	// optional terminator string
	argv.push_back(nullptr);

	return argv;
}

inline void OpenConsole()
{
	// Crea una nueva consola si no existe
	if (AllocConsole())
	{
		FILE* fp;
		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONOUT$", "w", stderr);
		freopen_s(&fp, "CONIN$", "r", stdin);

		// Opcional: sincroniza los streams de C++ con los de C
		std::ios::sync_with_stdio(true);

		// Opcional: mueve el cursor al final para evitar sobrescribir
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { 0, 0 });
	}
}

INT APIENTRY wWinMain(
	_In_ HINSTANCE hinstance, 
	_In_opt_ HINSTANCE hprevinstance, 
	_In_ PWSTR cmdline,
	_In_ INT cmdshow)
{
	int argc = 0;
	LPWSTR* argvw = CommandLineToArgvW(cmdline, &argc);

	const auto args = ConvertArgvWToArgvA(argvw, argc);

	//const auto console = GetStdHandle(STD_OUTPUT_HANDLE);
	OpenConsole();

	auto arguments = luma::Arguments{};
	arguments.parse(args);

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

		case DIRECTX:
		{
			luma::GPU gpu{ hinstance, 
				static_cast<int>(luma::_options.width), static_cast<int>(luma::_options.height) };
			gpu.loop();
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
