import std;

#include "olcPixelGameEngine.h"

#include "camera.h"
#include "vector.h"
#include "matrix.h"

namespace luma
{
	Camera::Camera(float fov, float nearclip, float farclip, std::uint32_t width, std::uint32_t height) noexcept
		: fov(fov), nearclip(nearclip), farclip(farclip), width{ width }, height{ height }
	{
		dir = cjl::vec3{ 0, 0, -1 };
		pos = cjl::vec3{ 0, 0, 0 };

       	projection = projection_inverse = view = view_inverse = cjl::identity();

		moved = true;
	}

	bool Camera::update(float ts, olc::PixelGameEngine& pge) noexcept
	{
		static constexpr auto movement_speed = .01f;
		static constexpr auto look_speed = .02f;
		static constexpr auto rotation_speed = .3f;

		const cjl::vec3 up{ 0.f, 1.f, 0.f };
		const cjl::vec3 right = cjl::cross(dir, up);

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
		
			 if (pge.GetKey(olc::Key::W).bHeld) { pos = cjl::subtract(pos, cjl::scale(dir, movement_speed * ts)); moved = true; }
		else if (pge.GetKey(olc::Key::S).bHeld) { pos = cjl::add(pos, cjl::scale(dir, movement_speed * ts)); moved = true; }

			 if (pge.GetKey(olc::Key::A).bHeld) { pos = cjl::subtract(pos, cjl::scale(right, movement_speed * ts)); moved = true; }
		else if (pge.GetKey(olc::Key::D).bHeld) { pos = cjl::add(pos, cjl::scale(right, movement_speed * ts)); moved = true; }

			 if (pge.GetKey(olc::Key::Q).bHeld) { pos = cjl::subtract(pos, cjl::scale(up, movement_speed * ts)); moved = true; }
		else if (pge.GetKey(olc::Key::E).bHeld) { pos = cjl::add(pos, cjl::scale(up, movement_speed * ts)); moved = true; }

		if (Δmouse.x != .0f || Δmouse.y != .0f)
		{
			float Δpitch = +Δmouse.y * rotation_speed,
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
		projection = cjl::perspective(cjl::radians(fov), static_cast<float>(width), static_cast<float>(height), nearclip, farclip);
		projection_inverse = cjl::inverse(projection);
	}

	void Camera::recompute_view(void) noexcept
	{
		const auto at = cjl::add(pos, dir);

		view = cjl::lookat(pos, at, cjl::vec3{ 0.f, 1.f, 0.f });
		view_inverse = cjl::inverse(view);
	}

	void Camera::recompute_rays(void) noexcept
	{
		rays.resize(width * height);

		for (auto y = 0u; y < height; y++)
		{
			for (auto x = 0u; x < width; x++)
			{
				const auto one = cjl::broadcast<2>(1.f);

				cjl::vec2 coordinate = { static_cast<float>(x) / width,
										 static_cast<float>(y) / height };

				// renormalization
				coordinate = cjl::scale(coordinate, 2.f);
				coordinate = cjl::subtract(coordinate, one);

				const auto extended = cjl::vec4{ coordinate[0], coordinate[1], 1.f, 1.f };
				const auto target = cjl::apply(projection_inverse, extended);

				const auto truncated = cjl::truncate(target);
				const auto scalar = 1 / target[3];

				const auto corrected = cjl::scale(truncated, scalar);
				const auto normalized = cjl::normalize(corrected);
				const auto padded = cjl::extend(normalized, 0.f);

				const auto ray = cjl::apply(view_inverse, padded);
				rays[y * width + x] = cjl::truncate(ray);
			}
		}
	}
}