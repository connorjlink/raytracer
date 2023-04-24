#ifndef RAYTRACER_HPP
#define RAYTRACER_HPP

#include <Walnut/Image.h>
#include <Walnut/Timer.h>
#include <Walnut/Random.h>

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

#include "Camera.hpp"

namespace rt
{
	struct Ray
	{
		glm::vec3 pos, dir;
	};

	struct Sphere
	{
		glm::vec3 pos;
		float radius;

		glm::vec3 diffuse;
		float metallic, roughness;
	};

	struct Intersection
	{
		glm::vec3 color;
		glm::vec3 pos;
		glm::vec3 normal;
		float distance, exit;
		Sphere* object;

		auto operator<=>(const Intersection& rhs) const = default;
	};

	class Renderer
	{
	public:
		bool accumulate = true;
		float frametime = 0.f;
		
		Walnut::Image* image = nullptr;
		std::uint32_t* imageData = nullptr;
		glm::vec3* accumulatedData = nullptr;
		float framecount = 1.f;

		Camera camera;

		Intersection* closest = nullptr;

		Sphere s{ { 0, -1, 0 }, 1.0f, { .8, .8, .8 }, 1, 0 };

		Sphere a{ { 0, 1000, 0}, 1000, { .6, .6, .6 }, 1, 0 };
		Sphere b{ { 1003, 0, 0}, 1000, { .6, .6, .6 }, 0, 1 };
		Sphere c{ { 0, 0, 1003}, 1000, { .6, .6, .6 }, 0, 1 };
		Sphere d{ { 0, -1003, 0}, 1000, { .6, .6, .6 }, 0, 1 };
		Sphere e{ { -1003, 0, 0}, 1000, { .6, .6, .6 }, 0, 1 };
		Sphere f{ { 0, 0, -1003}, 1000, { .6, .6, .6 }, 0, 1 };

		Sphere t{ { 2, -1, 0 }, 0.5f, { 0, 1, 0 }, 1, 0 };

		std::vector<Sphere> spheres{ s, t, a };

		glm::vec3 light{ 0, -1, 0 };

	public:
		Renderer(void) noexcept;
		void Render(void) noexcept;
		void Resize(std::uint32_t width, std::uint32_t height) noexcept;

	private:
		Intersection Miss(void) noexcept;
		Intersection TraceRay(glm::vec3 pos, glm::vec3 dir) noexcept;

		glm::vec3 PerPixel(std::uint32_t x, std::uint32_t y) noexcept;
	};
}

#endif