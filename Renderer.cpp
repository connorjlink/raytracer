import std;

#include "renderer.h"
#include "timer.h"
#include "random.h"

namespace
{
	std::uint32_t RGBA(cjl::vec4 in) noexcept
	{
		std::uint32_t out = 0;

		in = cjl::scale(in, 255.f);

		out |= (static_cast<int>(in[0]) << 0);
		out |= (static_cast<int>(in[1]) << 8);
		out |= (static_cast<int>(in[2]) << 16);
		out |= (static_cast<int>(in[3]) << 24);

		return out;
	}
}

namespace luma
{
	Renderer::Renderer(void) noexcept
		: camera(70.0f, 0.1f, 100.0f)
	{
	}

	Intersection Renderer::miss(void) noexcept
	{
		return { cjl::vec3(.6, .7, .95), cjl::vec3(0), cjl::vec3(0), 0, 0, nullptr };
	}

	Intersection Renderer::trace_ray(const Ray& ray) noexcept
	{
		static constexpr auto max = std::numeric_limits<float>::max();

		auto distance = max;

		Intersection intersection{};

		for (auto& sphere : spheres)
		{
			const auto diff = cjl::subtract(ray.pos, sphere.pos);

			const auto a = cjl::dot(ray.dir, ray.dir);
			const auto b = 2 * cjl::dot(diff, ray.dir);
			const auto c = cjl::dot(diff, diff) - (sphere.radius * sphere.radius);

			// descriminant
			const auto d = (b * b) - (4 * a * c);

			if (d > 0) [[unlikely]]
			{
				const auto t = (-b - std::sqrt(d)) / (2 * a);
				const auto e = (-b + std::sqrt(d)) / (2 * a);

				if (t > 0) [[likely]]
				{
					const auto progress = cjl::scale(ray.dir, t);
					const auto hit = cjl::add(ray.pos, progress);

					const auto toward = cjl::subtract(hit, sphere.pos);
					const auto normal = cjl::normalize(toward);

					if (t < distance)
					{
						distance = t;
						intersection = { sphere.diffuse, hit, normal, t, e, &sphere };
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

	cjl::vec3 Renderer::render_pixel(std::uint32_t x, std::uint32_t y) noexcept
	{
		cjl::vec3 direct{}, indirect{}, result{};

		auto dir = camera.rays[y * camera.width + x];
		auto pos = camera.pos;

		static constexpr auto offset = .001f;

		const auto noise = Random::vec3(-offset, offset);
		const auto dir_noised = cjl::add(dir, noise);

		auto ray = Ray{ camera.pos, dir_noised };
		auto intersection = trace_ray(ray);

		const auto first = intersection;

		auto multiplier = .5f;

		for (auto bounce = 0; bounce < _options.bounces; bounce++)
		{
			if (intersection == miss())
			{
				const cjl::vec3 sky(.6f, .7f, .9f);
				
				const auto sky_contribution = cjl::scale(sky, multiplier);
				direct = cjl::add(direct, sky_contribution);
				break;
			}

			const auto hit_contribution = cjl::scale(intersection.color, multiplier);
			direct = cjl::add(direct, hit_contribution);

			multiplier *= .5f;

			const auto extruded = cjl::scale(intersection.normal, .001f);
			pos = cjl::add(intersection.pos, extruded);

			const auto ddelta = .5f * intersection.object->roughness;
			dir = cjl::reflect(dir, intersection.normal + Walnut::Random::Vec3(-ddelta, ddelta));

			intersection = trace_ray(camera.pos, dir + Walnut::Random::Vec3(-offset, offset));
		}
		
		if (first != miss())
		{
			for (auto sample = 0; sample < _options.samples; sample++)
			{
				auto dir = Random::vec3_onsphere();

				if (cjl::dot(dir, first.normal) < 0)
					dir = cjl::invert(dir);

				const auto new_dir = cjl::add(first.normal, dir);
				const auto ray = Ray{ first.pos, new_dir };
				const auto cast = trace_ray(ray);

				const auto scalar = 1 / (2 * cjl::pi());
				const auto scaled = cjl::scale(cast.color, scalar);

				const auto contribution = cjl::scale(scaled, Random::real());
				indirect = cjl::add(indirect, contribution);
			}

			const auto divisor = 1 / _options.samples;
			indirect = cjl::scale(indirect, divisor);

			const auto weight = cjl::scale(first.color, 1 / cjl::pi());
			const auto unscaled = cjl::add(direct, indirect);

			result = cjl::multiply(unscaled, weight);
		}
		
		else
		{
			result = first.color;
		}

		auto tonemap = [&](cjl::vec3& color)
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

	void Renderer::Render(void) noexcept
	{
		cjl::Timer timer{};

		auto width = image->GetWidth(),
			 height = image->GetHeight();

		Resize(width, height);
		camera.OnResize(width, height);
		camera.OnUpdate(frametime);

		if (camera.moved)
		{
			frame_count = 1.f;

			delete[] accumulated_data;
			accumulated_data = new cjl::vec3[width * height]();
			camera.moved = false;
		}

		#pragma omp parallel for
		for (auto y = 0; y < height; ++y)
		{
			for (auto x = 0; x < width; ++x)
			{
				auto index = y * width + x;

				accumulated_data[index] += render_pixel(x, y);

				image_data[index] = RGBA({ accumulated_data[index] / frame_count, 1 });
			}
		}

		image->SetData(image_data);
		frametime = timer.milliseconds();
		frame_count += 1.f;
	}

	void Renderer::Resize(std::uint32_t width, std::uint32_t height) noexcept
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

		frame_count = 1.f;

		delete[] image_data;
		image_data = new uint32_t[width * height];

		delete[] accumulated_data;
		accumulated_data = new cjl::vec3[width * height]();
	}
}