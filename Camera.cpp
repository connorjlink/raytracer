import std;

#include "olcPixelGameEngine.h"

#include "flux/vector.h"

#include "camera.h"

// camera.cpp
// (c) 2025 Connor J. Link. All Rights Reserved.

namespace
{
	constexpr fx::mat4 inverse(fx::mat4& matrix)
	{
		const auto A2323 = matrix[2][2] * matrix[3][3] - matrix[2][3] * matrix[3][2];
		const auto A1323 = matrix[2][1] * matrix[3][3] - matrix[2][3] * matrix[3][1];
		const auto A1223 = matrix[2][1] * matrix[3][2] - matrix[2][2] * matrix[3][1];
		const auto A0323 = matrix[2][0] * matrix[3][3] - matrix[2][3] * matrix[3][0];
		const auto A0223 = matrix[2][0] * matrix[3][2] - matrix[2][2] * matrix[3][0];
		const auto A0123 = matrix[2][0] * matrix[3][1] - matrix[2][1] * matrix[3][0];
		const auto A2313 = matrix[1][2] * matrix[3][3] - matrix[1][3] * matrix[3][2];
		const auto A1313 = matrix[1][1] * matrix[3][3] - matrix[1][3] * matrix[3][1];
		const auto A1213 = matrix[1][1] * matrix[3][2] - matrix[1][2] * matrix[3][1];
		const auto A2312 = matrix[1][2] * matrix[2][3] - matrix[1][3] * matrix[2][2];
		const auto A1312 = matrix[1][1] * matrix[2][3] - matrix[1][3] * matrix[2][1];
		const auto A1212 = matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1];
		const auto A0313 = matrix[1][0] * matrix[3][3] - matrix[1][3] * matrix[3][0];
		const auto A0213 = matrix[1][0] * matrix[3][2] - matrix[1][2] * matrix[3][0];
		const auto A0312 = matrix[1][0] * matrix[2][3] - matrix[1][3] * matrix[2][0];
		const auto A0212 = matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0];
		const auto A0113 = matrix[1][0] * matrix[3][1] - matrix[1][1] * matrix[3][0];
		const auto A0112 = matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0];

		auto det = matrix[0][0] * (matrix[1][1] * A2323 - matrix[1][2] * A1323 + matrix[1][3] * A1223)
				 - matrix[0][1] * (matrix[1][0] * A2323 - matrix[1][2] * A0323 + matrix[1][3] * A0223)
				 + matrix[0][2] * (matrix[1][0] * A1323 - matrix[1][1] * A0323 + matrix[1][3] * A0123)
				 - matrix[0][3] * (matrix[1][0] * A1223 - matrix[1][1] * A0223 + matrix[1][2] * A0123);
		
		det = 1 / det;

		fx::mat4 out{};

		out[0][0] = det *  (matrix[1][1] * A2323 - matrix[1][2] * A1323 + matrix[1][3] * A1223);
		out[0][1] = det * -(matrix[0][1] * A2323 - matrix[0][2] * A1323 + matrix[0][3] * A1223);
		out[0][2] = det *  (matrix[0][1] * A2313 - matrix[0][2] * A1313 + matrix[0][3] * A1213);
		out[0][3] = det * -(matrix[0][1] * A2312 - matrix[0][2] * A1312 + matrix[0][3] * A1212);
		out[1][0] = det * -(matrix[1][0] * A2323 - matrix[1][2] * A0323 + matrix[1][3] * A0223);
		out[1][1] = det *  (matrix[0][0] * A2323 - matrix[0][2] * A0323 + matrix[0][3] * A0223);
		out[1][2] = det * -(matrix[0][0] * A2313 - matrix[0][2] * A0313 + matrix[0][3] * A0213);
		out[1][3] = det *  (matrix[0][0] * A2312 - matrix[0][2] * A0312 + matrix[0][3] * A0212);
		out[2][0] = det *  (matrix[1][0] * A1323 - matrix[1][1] * A0323 + matrix[1][3] * A0123);
		out[2][1] = det * -(matrix[0][0] * A1323 - matrix[0][1] * A0323 + matrix[0][3] * A0123);
		out[2][2] = det *  (matrix[0][0] * A1313 - matrix[0][1] * A0313 + matrix[0][3] * A0113);
		out[2][3] = det * -(matrix[0][0] * A1312 - matrix[0][1] * A0312 + matrix[0][3] * A0112);
		out[3][0] = det * -(matrix[1][0] * A1223 - matrix[1][1] * A0223 + matrix[1][2] * A0123);
		out[3][1] = det *  (matrix[0][0] * A1223 - matrix[0][1] * A0223 + matrix[0][2] * A0123);
		out[3][2] = det * -(matrix[0][0] * A1213 - matrix[0][1] * A0213 + matrix[0][2] * A0113);
		out[3][3] = det *  (matrix[0][0] * A1212 - matrix[0][1] * A0212 + matrix[0][2] * A0112);
	
		return out;
	}

	fx::vec3 cross(const fx::vec3& vector1, const fx::vec3& vector2)
	{
		return
		{
			vector1[1] * vector2[2] - vector1[2] * vector2[1],
			vector1[2] * vector2[0] - vector1[0] * vector2[2],
			vector1[0] * vector2[1] - vector1[1] * vector2[0]
		};
	}
}

namespace luma
{
	Camera::Camera(float fov, float nearclip, float farclip, std::uint32_t width, std::uint32_t height) noexcept
		: fov(fov), nearclip(nearclip), farclip(farclip), width{ width }, height{ height }
	{
		dir = fx::vec3{ 0, 0, -1 };
		pos = fx::vec3{ 0, 0, 0 };

		projection = projection_inverse = view = view_inverse = fx::identity();

		moved = false;

		recompute_projection();
		recompute_view();
		recompute_rays();
	}

	bool Camera::update(float ts, olc::PixelGameEngine& pge) noexcept
	{
		static constexpr auto movement_speed = .005f;
		static constexpr auto look_speed = .005f;
		static constexpr auto rotation_speed = .001f;

		moved = false;

		const fx::vec3 up{ 0.f, 1.f, 0.f };
		const auto forward = fx::normalize(fx::vec3{ -dir[0], 0.f, dir[2] });
		right = fx::cross(forward, up);


		auto fly_speed = movement_speed;

		if (pge.GetKey(olc::Key::SHIFT).bHeld) { fly_speed *= 10.f; }

			 if (pge.GetKey(olc::Key::W).bHeld) { pos = fx::add(pos, fx::scale(forward, fly_speed * ts)); moved = true; }
		else if (pge.GetKey(olc::Key::S).bHeld) { pos = fx::subtract(pos, fx::scale(forward, fly_speed * ts)); moved = true; }

			 if (pge.GetKey(olc::Key::A).bHeld) { pos = fx::subtract(pos, fx::scale(right, fly_speed * ts)); moved = true; }
		else if (pge.GetKey(olc::Key::D).bHeld) { pos = fx::add(pos, fx::scale(right, fly_speed * ts)); moved = true; }

			 if (pge.GetKey(olc::Key::Q).bHeld) { pos = fx::add(pos, fx::scale(up, fly_speed * ts)); moved = true; }
		else if (pge.GetKey(olc::Key::E).bHeld) { pos = fx::subtract(pos, fx::scale(up, fly_speed * ts)); moved = true; }

			 if (pge.GetKey(olc::Key::UP).bHeld)   { pitch -= rotation_speed * ts; moved = true; }
		else if (pge.GetKey(olc::Key::DOWN).bHeld) { pitch += rotation_speed * ts; moved = true; }

			 if (pge.GetKey(olc::Key::LEFT).bHeld)  { yaw += rotation_speed * ts; moved = true; }
		else if (pge.GetKey(olc::Key::RIGHT).bHeld) { yaw -= rotation_speed * ts; moved = true; }

			 if (pge.GetKey(olc::Key::R).bHeld) { depth += rotation_speed * ts; moved = true; }
		else if (pge.GetKey(olc::Key::F).bHeld) { depth -= rotation_speed * ts; moved = true; }

			 if (pge.GetKey(olc::Key::T).bPressed) { show_depth = !show_depth;  }

		/*const float Δpitch = -Δmouse.y * rotation_speed,
					    Δyaw   = +Δmouse.x * rotation_speed;*/

		// prevent gimbal lock by limiting pitch control
		pitch = std::clamp(pitch, -fx::pi() / 4, fx::pi() / 4);

		const auto cos_pitch = std::cos(pitch);
		const auto sin_pitch = std::sin(pitch);

		const auto cos_yaw = std::cos(yaw);
		const auto sin_yaw = std::sin(yaw);

		const auto base = fx::vec3{ 0.f, 0.f, -1.0f };

		// TODO: add quaternion system to 

		dir =
		{
			 (base[0]             * cos_yaw                       + base[2]             * sin_yaw),
			-(base[0] * sin_pitch * sin_yaw + base[1] * cos_pitch - base[2] * sin_pitch * cos_yaw),
			 (base[0] * cos_pitch * sin_yaw + base[1] * sin_pitch + base[2] * cos_pitch * cos_yaw),
		};


		//cjl::quat q = cjl::normalize(cjl::cross(cjl::angleAxis(-pitchDelta, right),
		//	cjl::angleAxis(-yawDelta, cjl::vec3{ 0, 1, 0 })));
		//dir = cjl::rotate(q, dir);
		

		if (moved)
		{
			recompute_view();
			recompute_rays();
		}

		return moved;
	}

	void Camera::recompute_projection(void) noexcept
	{
		projection = fx::perspective(fx::radians(fov), static_cast<float>(width), static_cast<float>(height), nearclip, farclip);
#pragma message("FIX THE INVERSE FUNCTION!")
		projection_inverse = ::inverse(projection);
	}

	void Camera::recompute_view(void) noexcept
	{
		const auto at = fx::add(pos, dir);

		view = fx::lookat(pos, at, fx::vec3{ 0.f, 1.f, 0.f });
#pragma message("FIX THE INVERSE FUNCTION!")
		view_inverse = ::inverse(view);
	}

	void Camera::recompute_rays(void) noexcept
	{
		rays.resize(width * height);

		for (auto y = 0u; y < height; y++)
		{
			for (auto x = 0u; x < width; x++)
			{
				const auto one = fx::broadcast<2>(1.f);

				fx::vec2 coordinate = { static_cast<float>(x) / width,
										static_cast<float>(y) / height };

				// renormalization
				coordinate = fx::scale(coordinate, 2.f);
				coordinate = fx::subtract(coordinate, one);

				const auto extended = fx::vec4{ coordinate[0], coordinate[1], 1.f, 1.f };
				const auto target = fx::apply(projection_inverse, extended);

				const auto truncated = fx::truncate(target);
				const auto scalar = 1 / target[3];

				const auto corrected = fx::scale(truncated, scalar);
				const auto normalized = fx::normalize(corrected);
				const auto padded = fx::extend(normalized, 0.f);

				const auto ray = fx::apply(view_inverse, padded);
				rays[y * width + x] = fx::truncate(ray);
			}
		}
	}
}