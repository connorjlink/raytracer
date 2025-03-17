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

		return intersection;
	}

	fx::vec3 Renderer::render_pixel(std::uint32_t x, std::uint32_t y) noexcept
	{
		fx::vec3 direct{}, indirect{}, result{};

		auto dir = camera.rays[y * camera.width + x];
		auto pos = camera.pos;

		//static constexpr auto offset = .001f;
		static constexpr auto offset = .0f;

		const auto noise = fx::Random::vec3(-offset, offset);
		const auto dir_noised = fx::add(dir, noise);

		const auto ray = Ray{ pos, dir_noised };
		auto intersection = trace_ray(ray);

		if (intersection.object != nullptr)
		{
			const auto scalar = (fx::dot(light, intersection.normal) + 1) / 2;
			intersection.color = fx::scale(intersection.color, scalar);
		}

		const auto first = intersection;

		auto fresnel = [&](auto&& intersection)
		{
			// fresnel's law
			const auto cos_incident = fx::dot(fx::invert(dir_noised), intersection.normal);
			const auto coefficient = intersection.metallic;
			const auto sin_theta_squared = coefficient * coefficient * (1 - (cos_incident * cos_incident));
			
			if (sin_theta_squared > 1.)
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

			return r;
		};


		for (auto bounce = 0u; bounce < _options.bounces; bounce++)
		{
			if (intersection.object == nullptr)
			{
				const fx::vec3 sky_color{ .6f, .7f, .9f };
				
				const auto miss_contribution = fresnel(intersection);
				const auto sky = fx::scale(sky_color, miss_contribution);
				direct = fx::add(direct, sky);

				break;
			}

			const auto hit_contribution = fresnel(intersection);
			const auto hit = fx::scale(intersection.color, hit_contribution);
			direct = fx::add(direct, hit);

			const auto extruded = fx::scale(intersection.normal, .001f);
			pos = fx::add(intersection.pos, extruded);

			const auto gain = intersection.object->roughness;
			const auto roughness_noise = fx::Random::vec3(-gain, gain);
			const auto normal = fx::add(intersection.normal, roughness_noise);
			const auto dir_noise = fx::Random::vec3(-offset, offset);

			dir = fx::reflect(dir, normal);
			dir = fx::add(dir, dir_noise);

			const auto ray = Ray{ camera.pos, dir };
			intersection = trace_ray(ray);
		}

		if (first != miss())
		{
			for (auto sample = 0u; sample < _options.samples; sample++)
			{
				auto dir = fx::Random::vec3_sphere();

				// ensure the noise points outward
				if (fx::dot(dir, first.normal) < 0)
				{
					dir = fx::invert(dir);
				}

				const auto new_dir = fx::add(first.normal, dir);
				const auto ray = Ray{ first.pos, new_dir };
				const auto cast = trace_ray(ray);

				const auto scalar = 1 / (2 * fx::pi());
				const auto scaled = fx::scale(cast.color, scalar);

				const auto contribution = fx::scale(scaled, fx::Random::real());
				indirect = fx::add(indirect, contribution);
			}

			const auto divisor = 1.f / _options.samples;
			indirect = fx::scale(indirect, divisor);

			const auto weight = fx::scale(first.color, 1 / fx::pi());
			const auto unscaled = fx::add(direct, indirect);

			result = fx::multiply(unscaled, weight);
		}
		
		else
		{
			result = first.color;
		}

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
		for (auto y = 0u; y < height; ++y)
		{
			for (auto x = 0u; x < width; ++x)
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
