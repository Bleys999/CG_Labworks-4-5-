#include "Input.h"

Input::Input()
{
}

void Input::OnKeyDown(WPARAM keyState)
{
    mKeys[keyState] = true;
}

void Input::OnKeyUp(WPARAM keyState)
{
    mKeys[keyState] = false;
}

void Input::OnMouseDown(int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;
    mMouseButtonDown = true;
}

void Input::OnMouseUp()
{
    mMouseButtonDown = false;
}

void Input::OnMouseMove(int x, int y, bool buttonDown)
{
    if (buttonDown)
    {
        mMouseDeltaX = x - mLastMousePos.x;
        mMouseDeltaY = y - mLastMousePos.y;
    }
    mLastMousePos.x = x;
    mLastMousePos.y = y;
}