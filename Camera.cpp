import std;

#include "olcPixelGameEngine.h"

#include "flux/vector.h"

#include "camera.h"

namespace luma
{
	Camera::Camera(float fov, float nearclip, float farclip, std::uint32_t width, std::uint32_t height) noexcept
		: fov(fov), nearclip(nearclip), farclip(farclip), width{ width }, height{ height }
	{
		dir = fx::vec3{ 0, 0, -1 };
		pos = fx::vec3{ 0, 0, 0 };

       	projection = projection_inverse = view = view_inverse = fx::identity();

		moved = true;
	}

	bool Camera::update(float ts, olc::PixelGameEngine& pge) noexcept
	{
		static constexpr auto movement_speed = .01f;
		static constexpr auto look_speed = .02f;
		static constexpr auto rotation_speed = .3f;

		const fx::vec3 up{ 0.f, 1.f, 0.f };
		const fx::vec3 right = fx::cross(dir, up);

		const auto mouse_pos = pge.GetMousePos();
		const auto Δmouse = (mouse_pos - mouse_pos_old) * look_speed;
		pge.DrawStringDecal(olc::vf2d{ 0.f, 0.f }, std::format("{}, {}", Δmouse.x, Δmouse.y));
		mouse_pos_old = mouse_pos;

		/*if (!Input::IsMouseButtonDown(MouseButton::Right))
		{
			Input::SetCursorMode(CursorMode::Normal);
			return false;
		}

		Input::SetCursorMode(CursorMode::Locked);*/

		moved = false;
		
			 if (pge.GetKey(olc::Key::W).bHeld) { pos = fx::subtract(pos, fx::scale(dir, movement_speed * ts)); moved = true; }
		else if (pge.GetKey(olc::Key::S).bHeld) { pos = fx::add(pos, fx::scale(dir, movement_speed * ts)); moved = true; }

			 if (pge.GetKey(olc::Key::A).bHeld) { pos = fx::subtract(pos, fx::scale(right, movement_speed * ts)); moved = true; }
		else if (pge.GetKey(olc::Key::D).bHeld) { pos = fx::add(pos, fx::scale(right, movement_speed * ts)); moved = true; }

			 if (pge.GetKey(olc::Key::Q).bHeld) { pos = fx::subtract(pos, fx::scale(up, movement_speed * ts)); moved = true; }
		else if (pge.GetKey(olc::Key::E).bHeld) { pos = fx::add(pos, fx::scale(up, movement_speed * ts)); moved = true; }

		if (Δmouse.x != .0f || Δmouse.y != .0f)
		{
			const float Δpitch = +Δmouse.y * rotation_speed,
				        Δyaw   = -Δmouse.x * rotation_speed;

			const auto cos_pitch = std::cos(Δpitch);
			const auto sin_pitch = std::sin(Δpitch);

			const auto cos_yaw = std::cos(Δyaw);
			const auto sin_yaw = std::sin(Δyaw);

			const auto dir_copy = dir;

			dir =
			{
				 dir_copy[0]             * cos_yaw                           + dir_copy[2]             * sin_yaw,
				 dir_copy[0] * sin_pitch * sin_yaw + dir_copy[1] * cos_pitch - dir_copy[2] * sin_pitch * cos_yaw,
				-dir_copy[0] * cos_pitch * sin_yaw + dir_copy[1] * sin_pitch + dir_copy[2] * cos_pitch * cos_yaw,
			};


			//cjl::quat q = cjl::normalize(cjl::cross(cjl::angleAxis(-pitchDelta, right),
			//	cjl::angleAxis(-yawDelta, cjl::vec3{ 0, 1, 0 })));
			//dir = cjl::rotate(q, dir);

			moved = true;
		}

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
		projection_inverse = fx::inverse(projection);
	}

	void Camera::recompute_view(void) noexcept
	{
		const auto at = fx::add(pos, dir);

		view = fx::lookat(pos, at, fx::vec3{ 0.f, 1.f, 0.f });
		view_inverse = fx::inverse(view);
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