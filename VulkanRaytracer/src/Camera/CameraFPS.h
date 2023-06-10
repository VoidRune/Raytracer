#pragma once
#include "glm/glm.hpp"
#include "../Vulkan/VKHeaders.h"

class CameraFPS
{
public:
	CameraFPS(GLFWwindow* window);
	~CameraFPS();

	void Update(double deltaTime);

	float theta = 0.0f;
	float phi = 0.0f;
	float pitch = 0.0f;
	float yaw = -90.0f;
	float roll = 0.0f;
	float fov = 70.0f;
	float nearPlane = 0.1f;
	float farPlane = 1000.0f;

	GLFWwindow* window;
	float sensitivity = 0.2f;
	double lastMouseX;
	double lastMouseY;
	bool moved = false;

	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 inverseProjection;
	glm::mat4 inverseView;

	glm::vec3 position;
	glm::vec3 forward;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 worldUp;

};