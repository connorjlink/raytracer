#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <vector>

namespace rt
{
	class Camera
	{
	public:
		Camera(float vfov, float nearclip, float farclip) noexcept;

		bool OnUpdate(float ts) noexcept;
		void OnResize(std::uint32_t width, std::uint32_t height) noexcept;

	private:
		void RecalculateProjection(void) noexcept;
		void RecalculateView(void) noexcept;
		void RecalculateRayDirections(void) noexcept;

	public:
		glm::mat4 projection{ 1.0f }, iprojection{ 1.0f }, view{ 1.0f }, iview{ 1.0f };

		float vfov = 70.0f;

		float nearclip = 0.1f, farclip = 100.0f;

		bool moved = false;

		glm::vec3 pos{ 0.0f, 0.5f, -2.0f },
				  dir{ 0.0f, 0.0f,  0.0f };

		std::vector<glm::vec3> rays;

		glm::vec2 lastmouse{ 0.0f, 0.0f };

		std::uint32_t width = 0, height = 0;
	};
}

#endif