#include "CameraFPS.h"
//#include "Window/Window.h"
#include <algorithm>

CameraFPS::CameraFPS(GLFWwindow* window)
{
    this->window = window;
    int windowWidth;
    int windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glfwGetCursorPos(window, &lastMouseX, &lastMouseY);

    position = { 0.0f, 1.0f, -1.0f };
    up = { 0.0f, 1.0f, 0.0f };
    forward = glm::normalize(glm::vec3{
    cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
    sin(glm::radians(pitch)),
    sin(glm::radians(yaw)) * cos(glm::radians(pitch))
        });
    right = glm::cross(forward, up);

    view = glm::lookAtLH(position, position + forward, up);
    projection = glm::perspectiveFov(fov, (float)windowWidth, (float)windowHeight, nearPlane, farPlane);
    inverseProjection = glm::inverse(projection);
    inverseView = glm::inverse(view);
}

CameraFPS::~CameraFPS()
{

}

void CameraFPS::Update(double deltaTime)
{
    moved = false;
    double mouseX;
    double mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) && 
        (mouseX != lastMouseX || mouseY != lastMouseY))
    {
        yaw += (mouseX - lastMouseX) * sensitivity;
        pitch += (mouseY - lastMouseY) * sensitivity;

        yaw = fmod(yaw, 360.0f);
        pitch = std::clamp(pitch, -89.0f, 89.0f);

        forward = glm::normalize(glm::vec3{
            cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
            sin(glm::radians(pitch)),
            sin(glm::radians(yaw)) * cos(glm::radians(pitch))
            });
        right = glm::cross(forward, up);

        moved = true;
    }
    lastMouseX = mouseX;
    lastMouseY = mouseY;

    glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };

    if (glfwGetKey(window, GLFW_KEY_W)) { velocity -= forward; moved = true; }
    if (glfwGetKey(window, GLFW_KEY_S)) { velocity += forward; moved = true; }
    if (glfwGetKey(window, GLFW_KEY_D)) { velocity -= right; moved = true; }
    if (glfwGetKey(window, GLFW_KEY_A)) { velocity += right; moved = true; }

    velocity.y = 0.0f;
    if (velocity != glm::vec3{ 0.0f, 0.0f, 0.0f })
        velocity = glm::normalize(velocity);

    if (glfwGetKey(window, GLFW_KEY_SPACE)) { velocity.y++; moved = true; }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) { velocity.y--; moved = true; }

    float speed = 2.0f;
    if (glfwGetKey(window, GLFW_KEY_F)) { speed = 10.0f; }
    if (glfwGetKey(window, GLFW_KEY_C)) { speed = 0.5f; }

    position += velocity * speed * (float)deltaTime;

    if (moved)
    {
        int windowWidth;
        int windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        view = glm::lookAtLH(position, position + forward, up);
        inverseView = glm::inverse(view);
    }
}