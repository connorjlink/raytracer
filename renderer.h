#ifndef LUMA_RENDERER_H
#define LUMA_RENDERER_H

#include "flux/types.h"
#include "camera.h"

namespace luma
{
	struct Ray
	{
		fx::vec3 pos, dir;
	};

	struct Sphere
	{
		fx::vec3 pos;
		float radius;

		fx::vec3 diffuse;
		float metallic, roughness;
	};

	struct Intersection
	{
		fx::vec3 color;
		fx::vec3 pos;
		fx::vec3 normal;
		float distance, exit;
		Sphere* object;

		auto operator<=>(const Intersection& rhs) const = default;
	};

	class Renderer
	{
	public:
		float frametime = 0.f;
		float frame_count = 1.f;
		
		bool accumulate = true;
		fx::vec3* accumulated_data = nullptr;

		Camera camera;

		Intersection* closest = nullptr;

		Sphere s{ { 0, 0, 10 }, 1.0f, { 1, 1, 0 }, 1, 1 };

		Sphere a{ { 0, 1000, 0 }, 1000, { .6, .6, .6 }, 1, 0 };
		Sphere b{ { 1003, 0, 0 }, 1000, { .6, .6, .6 }, 0, 1 };
		Sphere c{ { 0, 0, 1003 }, 1000, { .6, .6, .6 }, 0, 1 };
		Sphere d{ { 0, -1003, 0 }, 1000, { .6, .6, .6 }, 0, 1 };
		Sphere e{ { -1003, 0, 0 }, 1000, { .6, .6, .6 }, 0, 1 };
		Sphere f{ { 0, 0, -1003 }, 1000, { .6, .6, .6 }, 0, 1 };

		Sphere t{ { 2, -1, 0 }, 0.5f, { 0, 1, 0 }, 1, 0 };

		std::vector<Sphere> spheres{ s };

		fx::vec3 light{ 0, -1, 0 };

	public:
		Renderer(void) noexcept;
		void render_to(std::uint32_t*, olc::PixelGameEngine&) noexcept;

	private:
		Intersection miss(void) noexcept;
		Intersection trace_ray(const Ray&) noexcept;
		fx::vec3 render_pixel(std::uint32_t, std::uint32_t) noexcept;
	};
}

#endif