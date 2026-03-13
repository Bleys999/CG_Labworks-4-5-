#pragma once

struct Config
{
    static constexpr float CameraSpeed = 50.0f;
    static constexpr float CameraSensitivity = 0.2f;
    static constexpr float CameraFOV = 0.25f * 3.14159f;
    static constexpr float CameraNearZ = 1.0f;
    static constexpr float CameraFarZ = 1000.0f;
    static constexpr float ClearColorR = 0.48f;
    static constexpr float ClearColorG = 0.52f;
    static constexpr float ClearColorB = 0.80f;
    static constexpr float ClearColorA = 1.0f;
};