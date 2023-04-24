#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Walnut/Input/Input.h"

using namespace Walnut;

namespace rt
{
	Camera::Camera(float vfov, float nearclip, float farclip) noexcept
		: vfov(vfov), nearclip(nearclip), farclip(farclip)
	{
		dir = glm::vec3(0, 0, -1);
		pos = glm::vec3(0, 0, 6);
	}

	bool Camera::OnUpdate(float ts) noexcept
	{
		glm::vec2 mousePos = Input::GetMousePosition();
		glm::vec2 delta = (mousePos - lastmouse) * 0.002f;
		lastmouse = mousePos;

		if (!Input::IsMouseButtonDown(MouseButton::Right))
		{
			Input::SetCursorMode(CursorMode::Normal);
			return false;
		}

		Input::SetCursorMode(CursorMode::Locked);

		moved = false;

		constexpr glm::vec3 up(0.0f, 1.0f, 0.0f);
		glm::vec3 right = glm::cross(dir, up);

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

			glm::quat q = glm::normalize(glm::cross(glm::angleAxis(-pitchDelta, right),
				glm::angleAxis(-yawDelta, glm::vec3(0, 1, 0))));
			dir = glm::rotate(q, dir);

			moved = true;
		}

		if (moved)
		{
			RecalculateView();
			RecalculateRayDirections();
		}

		return moved;
	}

	void Camera::OnResize(std::uint32_t w, std::uint32_t h) noexcept
	{
		if (width == w && height == h)
			return;

		width = w;
		height = h;

		RecalculateProjection();
		RecalculateRayDirections();
	}

	void Camera::RecalculateProjection(void) noexcept
	{
		projection = glm::perspectiveFov(glm::radians(vfov), static_cast<float>(width), static_cast<float>(height), nearclip, farclip);
		iprojection = glm::inverse(projection);
	}

	void Camera::RecalculateView(void) noexcept
	{
		view = glm::lookAt(pos, pos + dir, glm::vec3(0, 1, 0));
		iview = glm::inverse(view);
	}

	void Camera::RecalculateRayDirections(void) noexcept
	{
		rays.resize(width * height);

		for (auto y = 0; y < height; y++)
		{
			for (auto x = 0; x < width; x++)
			{
				glm::vec2 coord = { static_cast<float>(x) / width,
									static_cast<float>(y) / height };

				coord = coord * 2.0f - 1.0f;

				glm::vec4 target = iprojection * glm::vec4(coord.x, coord.y, 1, 1);
				glm::vec3 ray = glm::vec3(iview * glm::vec4(glm::normalize(glm::vec3(target) / target.w), 0));
				rays[y * width + x] = ray;
			}
		}
	}
}