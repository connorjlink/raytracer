#ifndef LUMA_CAMERA_HPP
#define LUMA_CAMERA_HPP

#include "flux/types.h"

namespace luma
{
	class Camera
	{
	public:
		Camera(float, float, float, std::uint32_t, std::uint32_t) noexcept;
		bool update(float, olc::PixelGameEngine&) noexcept;

	private:
		void recompute_projection(void) noexcept;
		void recompute_view(void) noexcept;
		void recompute_rays(void) noexcept;

	public:
		fx::mat4 projection, projection_inverse, view, view_inverse;

		float fov = 70.0f;

		float nearclip = 0.1f, farclip = 100.0f;

		bool moved = false;

		float pitch = 0.f, yaw = 0.f;

		fx::vec3 pos{ 0.0f, 0.5f, 5.0f },
				 dir{ 0.0f, 0.0f, 1.0f };

		std::vector<fx::vec3> rays;

		olc::vi2d mouse_pos_old{ 0, 0 };

		std::uint32_t width = 0, height = 0;
	};
}

#endif