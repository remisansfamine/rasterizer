#include <cmath>

#include <common/maths.hpp>

mat4x4 mat4::translate(const float3& value)
{
    return {
        1.f, 0.f, 0.f, value.x,
        0.f, 1.f, 0.f, value.y,
        0.f, 0.f, 1.f, value.z,
        0.f, 0.f, 0.f, 1.f
    };
}

mat4x4 mat4::scale(const float3& value)
{
    return {
        value.x, 0.f, 0.f, 0.f,
        0.f, value.y, 0.f, 0.f,
        0.f, 0.f, value.z, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
}

mat4x4 mat4::rotateX(float angleRadians)
{
    float cos = cosf(angleRadians),sin = sinf(angleRadians);

    return {
        1.f, 0.f, 0.f, 0.f,
        0.f, cos,-sin, 0.f,
        0.f, sin, cos, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
}

mat4x4 mat4::rotateY(float angleRadians)
{
    float cos = cosf(angleRadians), sin = sinf(angleRadians);

    return {
        cos, 0.f, sin, 0.f,
        0.f, 1.f, 0.f, 0.f,
        -sin,0.f, cos, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
}

mat4x4 mat4::rotateZ(float angleRadians)
{
    float cos = cosf(angleRadians), sin = sinf(angleRadians);

    return {
        cos, -sin, 0.f, 0.f,
        sin, cos, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
}

mat4x4 mat4::frustum(float left, float right, float bottom, float top, float near, float far)
{
    return {
        (2.f * near) / (right - left), 0.f, (right + left) / (right - left), 0.f,
        0.f, (2.f * near) / (top - bottom), (top + bottom) / (top - bottom), 0.f,
        0.f, 0.f, - (far + near) / (far - near), - (2.f * far * near) / (far - near),
        0.f, 0.f, -1.f, 0.f
    };
}


mat4x4 mat4::perspective(float fovY, float aspect, float near, float far)
{
    float top = near * tanf(fovY * 0.5f);
    float right = top * aspect;

    return mat4::frustum(-right, right, -top, top, near, far);
}

float3 operator-(const float3& f)
{
    return { -f.x, -f.y, -f.z };
}

float magnitude(float3 v)
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

float3 normalized(float3 v)
{
    float magn = magnitude(v);
    return magn == 0 ? v : v / magn;
}

float3 getSphericalCoords(float r, float theta, float phi)
{
    return { r * sinf(theta) * cosf(phi), r * cosf(theta), r * sinf(theta) * sinf(phi) };
}

float min(float value1, float value2)
{
    return value1 <= value2 ? value1 : value2;
}

float max(float value1, float value2)
{
    return value1 <= value2 ? value2 : value1;
}

float saturate(float value)
{
    return min(max(value, 0.f), 1.f);
}

float dot(float3 v1, float3 v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}