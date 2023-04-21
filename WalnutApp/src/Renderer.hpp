#ifndef RAYTRACER_HPP
#define RAYTRACER_HPP

#include <Walnut/Image.h>
#include <Walnut/Timer.h>
#include <Walnut/Random.h>

#include <glm/glm.hpp>
#include <vector>
#include <execution>

#include "Camera.hpp"

namespace
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
		float distance;
		Sphere* object;

		auto operator<=>(const Intersection& rhs) const = default;
	};

	class Renderer
	{
	public:
		bool accumulate;
		float frametime = 0.f;
		
		Walnut::Image* image = nullptr;
		uint32_t* imageData = nullptr;
		glm::vec3* accumulatedData = nullptr;
		float framecount = 1.f;

		Camera camera;

		Sphere s{ { 0, -1, 0 }, 1.0f, { 1, 0, 1 }, 1.f, 0.f };
		Sphere f{ { 0, 100, 0}, 100.0f, { 0, 0, 1 }, 0.f, 0.f };
		Sphere t{ { 2, -1, 0 }, 0.5f, { 0, 0, 1 }, 1.f, 0.f };

		std::vector<Sphere> spheres{ s, f };

		glm::vec3 light{ -1, -1, -1 };

		std::vector<uint32_t> vertical, horizontal;

	public:
		Renderer() 
			: camera(70.0f, 0.1f, 100.0f)
		{
			std::iota(vertical.begin(), vertical.end(), 0);
			std::iota(horizontal.begin(), horizontal.end(), 0);
		}

		uint32_t RGBA(glm::vec4 in) noexcept
		{
			uint32_t out = 0;

			in *= 255;

			out |= (static_cast<int>(in.x) << 0);
			out |= (static_cast<int>(in.y) << 8);
			out |= (static_cast<int>(in.z) << 16);
			out |= (static_cast<int>(in.a) << 24);
			
			return out;
		}

		Intersection Miss(void) noexcept
		{
			return { glm::vec3(.6, .7, .95), glm::vec3(0), glm::vec3(0), 0, nullptr};
		}

		Intersection TraceRay(glm::vec3 pos, glm::vec3 dir) noexcept
		{
			std::vector<Intersection> intersections{};

			for (auto& sphere : spheres)
			{
				auto dif = pos - sphere.pos;

				auto a =     glm::dot(dir, dir);
				auto b = 2 * glm::dot(dif, dir);
				auto c =	 glm::dot(dif, dif) - (sphere.radius * sphere.radius);

				auto d = (b * b) - (4 * a * c);

				if (d > 0)
				{
					auto t = (-b - glm::sqrt(d)) / (2 * a);

					if (t > 0)
					{
						auto intersection = pos + (dir * t);
						auto normal = glm::normalize(intersection - sphere.pos);

						//auto intensity = glm::max(glm::dot(normal, -light), 0.f);
						//return (sphere.diffuse * intensity);

						intersections.emplace_back(Intersection{ sphere.diffuse, intersection, normal, t, &sphere });
					}
				}
			}

			if (intersections.size() == 0)
				return Miss();

			std::sort(intersections.begin(), intersections.end(), [&](auto&& a, auto&& b)
			{
				return (a.distance > b.distance);
			});

			return intersections.back();
		}

		glm::vec3 PerPixel(uint32_t x, uint32_t y) noexcept
		{
			const auto bounces = 4;
			
			glm::vec3 color(0);

			auto multiplier = 1.0f;

			auto dir = camera.rays[y * camera.width + x];
			auto pos = camera.pos;

			bool occluded = false;

			for (auto bounce = 0; bounce < bounces; bounce++)
			{
				auto idelta = .001f;
				
				auto intersection = TraceRay(camera.pos, dir + Walnut::Random::Vec3(-idelta, idelta));

				if (intersection == Miss())
				{
					glm::vec3 sky(.6f, .7f, .9f);
					color += (sky * multiplier);
					break;
				}
				else
				{
					intersection.color *= .5f;
				}

				auto ddelta = .05f * (1 - intersection.object->roughness);

				color += (intersection.color * multiplier);
				
				if (occluded) {
					//color *= .5f;
				}
					

				multiplier *= .5f;

				pos = intersection.pos + (intersection.normal * .001f);
				dir = glm::reflect(dir, intersection.normal) + Walnut::Random::Vec3(-ddelta, ddelta);

				auto shadow = TraceRay(pos, light);

				if (shadow != Miss())
				{
					//color *= .5f;
					occluded = true;
				}
			}

			return color;
		}

		void render(void) noexcept
		{
			Walnut::Timer timer;

			auto width = image->GetWidth();
			auto height = image->GetHeight();

			auto aspect = static_cast<float>(width) / height;

			resize(width, height);
			camera.OnResize(width, height);
			camera.OnUpdate(frametime);

			if (camera.moved)
			{
				framecount = 1.f;

				delete[] accumulatedData;
				accumulatedData = new glm::vec3[width * height]();
				camera.moved = false;
			}

			#pragma omp parallel for
			for (auto y = 0; y < height; ++y)
			{
				for (auto x = 0; x < width; ++x)
				{
					auto index = y * width + x;

					accumulatedData[index] += PerPixel(x, y);

					imageData[index] = RGBA({ accumulatedData[index] / framecount, 1 });
				}
			}

			image->SetData(imageData);
			frametime = timer.ElapsedMillis();
			framecount += 1.f;
		}

		void resize(uint32_t width, uint32_t height) noexcept
		{
			if (image != nullptr)
			{
				if (image->GetWidth() == width &&
					image->GetHeight() == height)
				{
					return;
				}

				image->Resize(width, height);
			}
			
			else
			{
				image = new Walnut::Image(width, height, Walnut::ImageFormat::RGBA);
			}

			framecount = 1.f;

			delete[] imageData;
			delete[] accumulatedData;
			imageData = new uint32_t[width * height];
			accumulatedData = new glm::vec3[width * height]();
		}
	};
}

#endif