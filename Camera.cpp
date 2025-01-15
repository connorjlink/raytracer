#include "camera.h"

namespace luma
{
	Camera::Camera(float vfov, float nearclip, float farclip) noexcept
		: fov(vfov), nearclip(nearclip), farclip(farclip)
	{
		dir = cjl::vec3{ 0, 0, -1 };
		pos = cjl::vec3{ 0, 0, 6 };

       	projection = projection_inverse = view = view_inverse = cjl::identity();
	}

	bool Camera::OnUpdate(float ts) noexcept
	{
		cjl::vec2 mousePos = Input::GetMousePosition();
		cjl::vec2 delta = (mousePos - lastmouse) * 0.002f;
		lastmouse = mousePos;

		if (!Input::IsMouseButtonDown(MouseButton::Right))
		{
			Input::SetCursorMode(CursorMode::Normal);
			return false;
		}

		Input::SetCursorMode(CursorMode::Locked);

		moved = false;

		constexpr cjl::vec3 up{ 0.0f, 1.0f, 0.0f };
		cjl::vec3 right = cjl::cross(dir, up);

		constexpr float speed = .01f;

			 if (Input::IsKeyDown(KeyCode::W)) { pos += dir * speed * ts; moved = true; }
		else if (Input::IsKeyDown(KeyCode::S)) { pos -= dir * speed * ts; moved = true; }

			 if (Input::IsKeyDown(KeyCode::A)) { pos -= right * speed * ts; moved = true; }
		else if (Input::IsKeyDown(KeyCode::D)) { pos += right * speed * ts; moved = true; }

			 if (Input::IsKeyDown(KeyCode::Q)) { pos -= up * speed * ts; moved = true; }
		else if (Input::IsKeyDown(KeyCode::E)) { pos += up * speed * ts; moved = true; }

		if (delta.x != .0f || delta.y != .0f)
		{
			constexpr auto rotation_speed = .3f;

			float pitchDelta = -delta.y * rotation_speed,
					yawDelta = +delta.x * rotation_speed;

			cjl::quat q = cjl::normalize(cjl::cross(cjl::angleAxis(-pitchDelta, right),
				cjl::angleAxis(-yawDelta, cjl::vec3{ 0, 1, 0 })));
			dir = cjl::rotate(q, dir);

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
		view = cjl::look_at(pos, pos + dir, cjl::vec3{ 0, 1, 0 });
		view_inverse = cjl::inverse(view);
	}

	void Camera::recompute_rays(void) noexcept
	{
		rays.resize(width * height);

		for (auto y = 0; y < height; y++)
		{
			for (auto x = 0; x < width; x++)
			{
				cjl::vec2 coord = { static_cast<float>(x) / width,
									static_cast<float>(y) / height };

				coord = coord * 2.0f - 1.0f;

				cjl::vec4 target = projection_inverse * cjl::vec4{ coord.x, coord.y, 1, 1 };
				cjl::vec3 ray = cjl::vec3(iview * glm::vec4(glm::normalize(glm::vec3(target) / target.w), 0));
				rays[y * width + x] = ray;
			}
		}
	}
}