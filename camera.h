#ifndef LUMA_CAMERA_HPP
#define LUMA_CAMERA_HPP

#include "base.h"
#include "types.h"

namespace luma
{
	class Camera
	{
	public:
		Camera(float, float, float) noexcept;
		bool OnUpdate(float) noexcept;

	private:
		void recompute_projection(void) noexcept;
		void recompute_view(void) noexcept;
		void recompute_rays(void) noexcept;

	public:
		cjl::mat4 projection, projection_inverse, view, view_inverse;

		float fov = 70.0f;

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