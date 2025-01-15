#include "renderer.h"

namespace
{
	std::uint32_t RGBA(cjl::vec4 in) noexcept
	{
		std::uint32_t out = 0;

		in *= 255;

		out |= (static_cast<int>(in.x) << 0);
		out |= (static_cast<int>(in.y) << 8);
		out |= (static_cast<int>(in.z) << 16);
		out |= (static_cast<int>(in.a) << 24);

		return out;
	}
}

namespace
{
	constexpr auto max = std::numeric_limits<float>::max();
	constexpr auto PI = 3.14159265f;

	constexpr auto bounces = 2;
	constexpr auto samples = 2;
}

namespace rt
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
		auto distance = max;

		Intersection intersection{};

		for (auto& sphere : spheres)
		{
			const auto diff = ray.pos - sphere.pos;

			const auto a = cjl::dot(ray.dir, ray.dir);
			const auto b = 2 * cjl::dot(dif, ray.dir);
			const auto c = cjl::dot(diff, diff) - (sphere.radius * sphere.radius);

			// descriminant
			const auto d = (b * b) - (4 * a * c);

			if (d > 0) [[unlikely]]
			{
				const auto t = (-b - cjl::sqrt(d)) / (2 * a);
				const auto e = (-b + cjl::sqrt(d)) / (2 * a);

				if (t > 0) [[likely]]
				{
					const auto hit = ray.pos + (ray.dir * t);
					const auto normal = cjl::normalize(hit - sphere.pos);

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

	cjl::vec3 Renderer::PerPixel(std::uint32_t x, std::uint32_t y) noexcept
	{
		cjl::vec3 direct{}, indirect{}, result{};

		auto dir = camera.rays[y * camera.width + x];
		auto pos = camera.pos;

		constexpr auto offset = .001f;
		auto intersection = TraceRay(camera.pos, dir + Walnut::Random::Vec3(-offset, offset));

		const auto first = intersection;

		auto multiplier = .5f;

		for (auto bounce = 0; bounce < bounces; bounce++)
		{
			if (intersection == Miss())
			{
				const scjl::vec3 sky(.6f, .7f, .9f);
				direct += (sky * multiplier);
				break;
			}

			direct += (intersection.color * multiplier);
			multiplier *= .5f;

			pos = intersection.pos + (intersection.normal * .001f);

			const auto ddelta = .5f * intersection.object->roughness;
			dir = cjl::reflect(dir, intersection.normal + Walnut::Random::Vec3(-ddelta, ddelta));

			intersection = TraceRay(camera.pos, dir + Walnut::Random::Vec3(-offset, offset));
		}
		
		if (first != miss())
		{
			for (auto sample = 0; sample < samples; sample++)
			{
				auto ray = Walnut::Random::InUnitSphere();

				if (cjl::dot(ray, first.normal) < 0)
					ray = -ray;

				const auto cast = TraceRay(first.pos, first.normal + ray);

				indirect += Walnut::Random::Float() * cast.color / (1 / (2 * PI));
			}

			indirect /= samples;
			result = (direct + indirect) * first.color / PI;
		}
		
		else
		{
			result = first.color;
		}

		auto tonemap = [&](cjl::vec3 in)
		{
			auto func = [&](float in)
			{
				return 1 - std::exp(-in);
			};

			auto out = in;

			out.x = func(out.x);
			out.y = func(out.y);
			out.z = func(out.z);

			return out;
		};

		result = tonemap(result);

		return result;
	}

	void Renderer::Render(void) noexcept
	{
		Walnut::Timer timer;

		auto width = image->GetWidth(),
			 height = image->GetHeight();

		Resize(width, height);
		camera.OnResize(width, height);
		camera.OnUpdate(frametime);

		if (camera.moved)
		{
			framecount = 1.f;

			delete[] accumulatedData;
			accumulatedData = new cjl::vec3[width * height]();
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

		framecount = 1.f;

		delete[] imageData;
		imageData = new uint32_t[width * height];

		delete[] accumulatedData;
		accumulatedData = new cjl::vec3[width * height]();
	}
}