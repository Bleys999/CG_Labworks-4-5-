#pragma once

#include <windows.h>

class Input
{
public:
    Input();
    ~Input() = default;

    void OnKeyDown(WPARAM keyState);
    void OnKeyUp(WPARAM keyState);
    void OnMouseDown(int x, int y);
    void OnMouseUp();
    void OnMouseMove(int x, int y, bool buttonDown);

    bool IsKeyDown(WPARAM key) const { return mKeys[key]; }
    int GetMouseDeltaX() const { return mMouseDeltaX; }
    int GetMouseDeltaY() const { return mMouseDeltaY; }
    bool IsMouseButtonDown() const { return mMouseButtonDown; }
    void ResetMouseDelta() { mMouseDeltaX = mMouseDeltaY = 0; }
    POINT GetLastMousePos() const { return mLastMousePos; }

private:
    bool mKeys[256] = { false };
    int mMouseDeltaX = 0;
    int mMouseDeltaY = 0;
    bool mMouseButtonDown = false;
    POINT mLastMousePos = { 0, 0 };
};