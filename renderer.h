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

	struct Material
	{
		fx::vec3 diffuse;
		float albedo; // controls the amount of indirect light recieved
		float metallic; // controls the strength of reflections
		float roughness; // controls the dispersion of reflections
	};

	struct Sphere
	{
		fx::vec3 pos;
		float radius;
		Material material;
	};


	struct Intersection
	{
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

		Sphere s{ {  0,  0, -10 }, 2.0f, { { 0, 0, 1 }, .5, .00001, 1 } };
		Sphere q{ {  4,  0, -10 }, 1.0f, { { 0, 1, 0 }, 1, .5, .1 } };
		Sphere r{ { -2, -4, -10 }, 1.0f, { { 1, 0, 0 }, 1, 1, 1 } };
		Sphere u{ { -4, -4, -10 }, 1.0f, { { 1, 0, 0 }, .1, .00001, 0 } };

		Sphere a{ { 0, 102, 0 }, 100, { { .6, .6, .6 }, 1, .1, .5 } };

		std::vector<Sphere> spheres{ s, a, q, r, u };

		fx::vec3 light{ -1, -1, 0 };

	public:
		Renderer(void) noexcept;
		void render_to(std::uint32_t*, olc::PixelGameEngine*) noexcept;

	private:
		Intersection miss(void) noexcept;
		fx::vec3 direct_illumination(const Intersection&) noexcept;
		fx::vec3 indirect_illumination(const Intersection&) noexcept;
		Ray reflect_intersection(const Intersection&, const Ray&) noexcept;
		Intersection trace_ray(const Ray&) noexcept;
		fx::vec3 render_pixel(std::uint32_t, std::uint32_t) noexcept;
	};
}

#endif