#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <vector>

class Camera
{
public:
	Camera(float verticalFOV, float nearClip, float farClip);

	bool OnUpdate(float ts);
	void OnResize(uint32_t width, uint32_t height);

private:
	void RecalculateProjection();
	void RecalculateView();
	void RecalculateRayDirections();
public:
	glm::mat4 projection{ 1.0f };
	glm::mat4 view{ 1.0f };
	glm::mat4 iprojection{ 1.0f };
	glm::mat4 iview{ 1.0f };

	float vfov = 45.0f;
	float nearclip = 0.1f;
	float farclip = 100.0f;

	bool moved = false;

	const float rotation_speed = 0.3f;

	glm::vec3 pos{ 0.0f, 0.5f, -2.0f };
	glm::vec3 dir{ 0.0f, 0.0f, 0.0f };

	std::vector<glm::vec3> rays;

	glm::vec2 lastmouse{ 0.0f, 0.0f };

	uint32_t width = 0, height = 0;
};

#endif