import std;

#include "olcPixelGameEngine.h"

#include "flux/timer.h"
#include "flux/random.h"
#include "flux/vector.h"

#include "renderer.h"
#include "arguments.h"

namespace
{
	std::uint32_t RGBA(fx::vec4 in) noexcept
	{
		std::uint32_t out = 0;

		in = fx::scale(in, 255.f);

		out |= (static_cast<int>(in[0]) << 0);
		out |= (static_cast<int>(in[1]) << 8);
		out |= (static_cast<int>(in[2]) << 16);
		out |= (static_cast<int>(in[3]) << 24);

		return out;
	}

	std::uint32_t RGB(fx::vec3 in) noexcept
	{
		std::uint32_t out = 0;

		in = fx::scale(in, 255.f);

		out |= (static_cast<int>(in[0]) << 16);
		out |= (static_cast<int>(in[1]) << 8);
		out |= (static_cast<int>(in[2]) << 0);

		return out;
	}
}

namespace
{
	template<typename T>
	class SquareRoot
	{
	public:
		T operator()(T value) const noexcept
		{
			return std::sqrt(value);
		}
	};
}

namespace
{
	static constexpr auto OFFSET = .001f;
}

namespace luma
{
	Renderer::Renderer(void) noexcept
		: camera(70.0f, 0.1f, 100.0f, _options.width, _options.height)
	{
		const auto size = _options.width * _options.height;

		delete[] accumulated_data;
		accumulated_data = new fx::vec3[size]();
	}

	Intersection Renderer::miss(void) noexcept
	{
		return { fx::vec3(.6f, .7f, .95f), 1.f, 1.f, fx::vec3{ 0.f, 0.f, 0.f }, fx::vec3{ 0.f, 0.f, 0.f }, 0, 0, nullptr};
	}

	fx::vec3 Renderer::direct_illumination(const Intersection& intersection) noexcept
	{


		return fx::vec3();
	}

	fx::vec3 Renderer::indirect_illumination(const Intersection& intersection) noexcept
	{
		fx::vec3 out{};

		for (auto sample = 0u; sample < _options.samples; sample++)
		{
			auto dir = fx::Random::vec3_sphere();

			// ensure the noise points outward
			if (fx::dot(dir, intersection.normal) < 0)
			{
				dir = fx::invert(dir);
			}

			const auto new_dir = fx::add(intersection.normal, dir);
			const auto ray = Ray{ intersection.pos, new_dir };
			const auto cast = trace_ray(ray);

			out = fx::add(out, cast.color);
		}

		const auto divisor = 1.f / _options.samples;
		out = fx::scale(out, divisor);

		return out;
	}

	Ray Renderer::reflect_intersection(const Intersection& intersection, const Ray& ray) noexcept
	{
		// need to ensure that the reflection ray doesn't re-hit the same object due to being inside (floating point inaccuracy)
		const auto extruded = fx::scale(intersection.normal, .001f);
		auto pos = fx::add(intersection.pos, extruded);

		const auto gain = intersection.object->roughness;
		const auto roughness_noise = fx::Random::vec3(-gain, gain);
		const auto normal = fx::add(intersection.normal, roughness_noise);
		const auto dir_noise = fx::Random::vec3(-OFFSET, OFFSET);

		auto dir = fx::reflect(ray.dir, normal);
		//dir = fx::add(dir, dir_noise);

		return Ray{ pos, dir };
	}

	Intersection Renderer::trace_ray(const Ray& ray) noexcept
	{
		static constexpr auto max = std::numeric_limits<float>::max();

		auto distance = max;

		Intersection intersection{};

		for (auto& sphere : spheres)
		{
			const auto diff = fx::subtract(ray.pos, sphere.pos);

			const auto a = fx::dot(ray.dir, ray.dir);
			const auto b = 2 * fx::dot(diff, ray.dir);
			const auto c = fx::dot(diff, diff) - (sphere.radius * sphere.radius);

			// descriminant
			const auto d = (b * b) - (4 * a * c);

			if (d > 0) [[unlikely]]
			{
				const auto t = (-b - std::sqrt(d)) / (2 * a);
				const auto e = (-b + std::sqrt(d)) / (2 * a);

				if (t > 0) [[likely]]
				{
					const auto progress = fx::scale(ray.dir, t);
					const auto hit = fx::add(ray.pos, progress);

					const auto toward = fx::subtract(hit, sphere.pos);
					const auto normal = fx::normalize(toward);

					if (t < distance)
					{
						distance = t;
						intersection = { sphere.diffuse, sphere.metallic, sphere.roughness, hit, normal, t, e, &sphere };
					}
				}
			}
		}

		//no object was hit
		if (distance == max) [[likely]]
		{
			return miss();
		}

#ifdef SIMPLE_SHADOWS
		const auto scalar = std::clamp(fx::dot(light, intersection.normal), 0.f, 1.0f);
		intersection.color = fx::scale(intersection.color, scalar);
#endif

		return intersection;
	}

	fx::vec3 Renderer::render_pixel(std::uint32_t x, std::uint32_t y) noexcept
	{
		fx::vec3 direct{}, indirect{}, result{};

		auto dir = camera.rays[y * camera.width + x];
		auto pos = camera.pos;

		const auto noise = fx::Random::vec3(-OFFSET, OFFSET);
		const auto dir_noised = fx::add(dir, noise);

		auto compose = [&](auto&& color1, auto&& color2, auto&& reflectivity)
		{
			const auto t1 = fx::scale(color1, 1 - reflectivity);
			const auto t2 = fx::scale(color2, reflectivity);

			const auto total = fx::add(t1, t2);
			return total;
		};

		auto tonemap = [&](fx::vec3& color)
		{
			auto func = [&](float in)
			{
				return 1 - std::exp(-in);
			};

			color[0] = func(color[0]);
			color[1] = func(color[1]);
			color[2] = func(color[2]);
		};

		auto mix = [&](auto&& color1, auto&& color2, auto&& reflectivity)
		{
			const auto color1_squared = fx::multiply(color1, color1);
			const auto color2_squared = fx::multiply(color2, color2);

			const auto t1 = 1 - reflectivity;
			const auto t2 = reflectivity;

			const auto a = fx::scale(color1_squared, t1);
			const auto b = fx::scale(color2_squared, t2);

			const auto total_squared = fx::add(a, b);
			const auto total = fx::_accumulate<SquareRoot<fx::platform_type>>(total_squared);

			return total;
		};

		auto lerp = [&](auto&& color1, auto&& color2, auto&& t)
		{
			const auto t1 = 1 - t;
			const auto t2 = t;

			const auto a = fx::scale(color1, t1);
			const auto b = fx::scale(color2, t2);

			const auto total = fx::add(a, b);

			return total;
		};

		auto fresnel = [&](auto&& intersection, bool invert)
		{
			auto ray = dir_noised;

			if (invert)
			{
				ray = fx::invert(ray);
			}

			// fresnel's law
			const auto cos_incident = fx::dot(ray, intersection.normal);
			const auto coefficient = intersection.metallic;
			const auto sin_theta_squared = coefficient * coefficient * (1 - (cos_incident * cos_incident));
			
			if (sin_theta_squared >= 1.f)
			{
				// total internal reflection
				return 1.f;
			}

			const auto cos_theta = std::sqrt(1 - sin_theta_squared);

			// schlick approximation
			const auto r0 = (1 - coefficient) / (1 + coefficient);
			const auto r0_squared = r0 * r0;

			const auto partial = 1 - cos_incident;
			const auto power = partial * partial * partial * partial * partial;

			const auto r = r0 + (1 - r0) * power;

			return 1 - std::clamp(r, 0.f, 1.f);
		};


		Intersection intersection{}, first{};

		for (auto bounce = 0u; bounce < _options.bounces; bounce++)
		{
			const auto ray = Ray{ camera.pos, dir };
			intersection = trace_ray(ray);

			if (bounce == 0)
			{
				first = intersection;
			}

			if (intersection.object == nullptr)
			{
				const fx::vec3 top_sky_color{ .529f, .808f, .922f };
				const fx::vec3 bottom_sky_color{ .106f, .275f, .711f };

				const auto clamped = std::clamp(dir[1], -1.f, 1.f);
				const auto adjusted = (clamped + 1.f) * .5f;

				const auto t = lerp(top_sky_color, bottom_sky_color, adjusted);

				const auto reflectivity = fresnel(intersection, true);
				direct = mix(direct, t, reflectivity);

				break;
			}

			

			dir = reflect_intersection(intersection, ray).dir;

			auto reflection = trace_ray(Ray{ intersection.pos, dir });


			const auto diffuse = intersection.color;
			const auto reflected = reflection.color;

			const auto reflectivity = fresnel(intersection, true);

			const auto nonspecular = mix(diffuse, reflected, reflectivity);

			const auto indirect = indirect_illumination(intersection);


			const auto t = fx::add(nonspecular, indirect);
			direct = mix(direct, t, .5f);

			if (intersection.metallic == 0)
			{
				break;
			}
		}

		if (first.object != nullptr)
		{
			/*const auto indirect = indirect_illumination(first);

			const auto weight = fx::scale(first.color, 1 / fx::pi());
			const auto unscaled = fx::add(direct, indirect);
			result = fx::multiply(unscaled, weight);*/

			/*const auto d = first.metallic;
			const auto i = 1 - d;

			const auto sum = fx::add(fx::scale(direct, d), fx::scale(indirect, i));
			result = sum;*/
			result = direct;
		}
		
		// no need to waste time with indirect light no object was hit
		else
		{
			result = direct;
		}

		tonemap(result);
		return result;
	}

	void Renderer::render_to(std::uint32_t* target, olc::PixelGameEngine& pge) noexcept
	{
		fx::Timer timer{};

		const auto width = _options.width;
		const auto height = _options.height;

		camera.update(frametime, pge);

		if (camera.moved)
		{
			frame_count = 1.f;

			delete[] accumulated_data;
			accumulated_data = new fx::vec3[width * height]();

			for (auto i = 0u; i < width * height; i++)
			{
				accumulated_data[i] = fx::broadcast<3>(0.f);
			}

			camera.moved = false;
		}

	//#pragma omp parallel for
		for (auto y = 0; y < height; ++y)
		{
			for (auto x = 0; x < width; ++x)
			{
//#define TESTING
#ifndef TESTING
				auto index = (y * width) + x;

				const auto result = render_pixel(x, y);
				
				auto& data = accumulated_data[index];
				data = fx::add(data, result);

				const auto mean = fx::scale(accumulated_data[index], 1 / frame_count);
				target[index] = RGB(mean);
#else
				const auto index = (y * width) + x;
				target[index] = RGB(render_pixel(x, y));
#endif
			}
		}

		frametime = timer.milliseconds();
		frame_count += 1.f;
	}
}
