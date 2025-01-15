#ifndef LUMA_CAMERA_HPP
#define LUMA_CAMERA_HPP

#include <vector>

#include "vector.h"
#include "matrix.h"

namespace luma
{
	class Camera
	{
	public:
		Camera(float, float, float) noexcept;
		bool OnUpdate(float) noexcept;

	private:
		void RecalculateProjection(void) noexcept;
		void RecalculateView(void) noexcept;
		void RecalculateRayDirections(void) noexcept;

	public:
		cjl::mat4 projection{ 1.0f }, iprojection{ 1.0f }, view{ 1.0f }, iview{ 1.0f };

		float vfov = 70.0f;

		float nearclip = 0.1f, farclip = 100.0f;

		bool moved = false;

		cjl::vec3 pos{ 0.0f, 0.5f, -2.0f },
				  dir{ 0.0f, 0.0f,  0.0f };

		std::vector<cjl::vec3> rays;

		cjl::vec2 lastmouse{ 0.0f, 0.0f };

		std::uint32_t width = 0, height = 0;
	};
}

#endif