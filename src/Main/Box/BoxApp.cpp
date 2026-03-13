#include "../../Common/d3dApp.h"
#include "../../Common/Camera.h"
#include "../../Common/Scene.h"
#include "../../Common/Renderer.h"
#include "../../Common/Input.h"
#include "../../Common/Config.h"

using namespace DirectX;

class BoxApp : public D3DApp
{
public:
    BoxApp(HINSTANCE hInstance);
    virtual bool Initialize() override;
    virtual void Update(const GameTimer& gt) override;
    virtual void Draw(const GameTimer& gt) override;

private:
    virtual void OnResize() override;
    virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y) override;
    virtual void OnKeyDown(WPARAM keyState, int x, int y) override;
    virtual void OnKeyUp(WPARAM keyState, int x, int y) override;

private:
    Camera mCamera;
    Scene mScene;
    Renderer mRenderer;
    Input mInput;
};

BoxApp::BoxApp(HINSTANCE hInstance) : D3DApp(hInstance) {}

bool BoxApp::Initialize()
{
    if (!D3DApp::Initialize()) return false;

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    if (!mRenderer.Initialize(md3dDevice.Get())) return false;
    if (!mScene.Initialize(md3dDevice.Get(), mCommandList.Get())) return false;

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, cmdsLists);
    FlushCommandQueue();

    return true;
}

void BoxApp::OnResize()
{
    D3DApp::OnResize();
    mCamera.SetLens(Config::CameraFOV, AspectRatio(),
        Config::CameraNearZ, Config::CameraFarZ);
}

void BoxApp::Update(const GameTimer& gt)
{
    mCamera.HandleInput(gt, mInput);
    mInput.ResetMouseDelta();
    mScene.Update(gt, mCamera);
}

void BoxApp::Draw(const GameTimer& gt)
{
    ThrowIfFailed(mDirectCmdListAlloc->Reset());
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mRenderer.GetPSO()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &barrier);

    mCommandList->ClearRenderTargetView(CurrentBackBufferView(),
        Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    mRenderer.Apply(mCommandList.Get());
    mScene.Draw(mCommandList.Get());

    auto barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    mCommandList->ResourceBarrier(1, &barrier2);

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, cmdsLists);

    Present();
    FlushCommandQueue();
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mInput.OnMouseDown(x, y); SetCapture(mhMainWnd);
}
void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    mInput.OnMouseUp(); ReleaseCapture();
}
void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    mInput.OnMouseMove(x, y, (btnState & MK_LBUTTON) != 0);
}
void BoxApp::OnKeyDown(WPARAM keyState, int x, int y)
{
    mInput.OnKeyDown(keyState);
}
void BoxApp::OnKeyUp(WPARAM keyState, int x, int y)
{
    mInput.OnKeyUp(keyState);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        BoxApp theApp(hInstance);
        if (!theApp.Initialize())
            return 0;
        return theApp.Run();
    }
    catch (DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}