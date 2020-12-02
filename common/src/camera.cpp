
#include <imgui.h>

#include <common/maths.hpp>

#include <common/camera.hpp>

#define M_PI 3.14159265358979323846
#include <cmath>

Camera::Camera(int width, int height)
{
    aspect = (float)width / (float)height;
}

void Camera::update(float deltaTime, const CameraInputs& inputs)
{
    const float MOUSE_SENSITIVITY = 0.5f;

    speed += (inputs.speedUp - inputs.speedDown) * deltaTime;

    yaw += inputs.deltaX * MOUSE_SENSITIVITY * deltaTime;
    pitch += inputs.deltaY * MOUSE_SENSITIVITY * deltaTime;

    int xAxis = inputs.moveForward - inputs.moveBackward;
    int yAxis = inputs.moveUpward - inputs.moveDownward;
    int zAxis = inputs.moveRight - inputs.moveLeft;

    position.x += (sinf(yaw) * xAxis + cosf(yaw) * zAxis) * speed * deltaTime;
    position.y += yAxis * speed * deltaTime;
    position.z += (sinf(yaw) * zAxis - cosf(yaw) * xAxis) * speed * deltaTime;
}

mat4x4 Camera::getViewMatrix()
{
    return mat4::rotateX(pitch) * mat4::rotateY(yaw) * mat4::translate(-position);
}

mat4x4 Camera::getProjection()
{
    return mat4::perspective(fovY * M_PI / 180.f, aspect, near, far);
}

void Camera::showImGuiControls()
{
    ImGui::SliderFloat("FOV", &fovY, 0.f, 180.f);
    ImGui::SliderFloat("near", &near, 0.001f, 1.f);
    ImGui::SliderFloat("far", &far, 0.75f, 500.f);
}