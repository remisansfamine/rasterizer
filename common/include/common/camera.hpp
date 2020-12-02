#pragma once

#include <common/types.hpp>

struct CameraInputs
{
    float deltaX;
    float deltaY;
    bool moveForward;
    bool moveBackward;
    bool moveLeft;
    bool moveRight;
    bool moveUpward;
    bool moveDownward;
    bool speedUp;
    bool speedDown;
    // ... and more if needed
};

struct Camera
{
    Camera(int width, int height);

    void update(float deltaTime, const CameraInputs& inputs);
    mat4x4 getViewMatrix();
    mat4x4 getProjection();
    float3 position = {0.f, 0.f, 0.f};

    float aspect;
    float near = 0.001f;
    float far = 200.f;
    float fovY = 60.f;
    float yaw = 0.f;
    float pitch = 0.f;

    float speed = 0.3f;

    void showImGuiControls();
};