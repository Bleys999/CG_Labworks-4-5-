#include "../Common/Core/d3dApp.h"
#include "../Common/Scene/Camera.h"
#include "../Common/Scene/Scene.h"
#include "../Common/D3D12/Renderer.h"
#include "../Common/Core/Input.h"
#include "../Common/Assets/TextureManager.h"
#include "../Common/Core/Config.h"

using namespace DirectX;

class MainApp : public D3DApp
{
public:
    MainApp(HINSTANCE hInstance);
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
    TextureManager mTextureManager;
};

MainApp::MainApp(HINSTANCE hInstance) : D3DApp(hInstance) {}

bool MainApp::Initialize()
{
    if (!D3DApp::Initialize()) return false;

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    mCamera.SetPosition(0.0f, 5.0f, -20.0f);
    mCamera.SetSpeed(Config::CameraSpeed);
    mCamera.SetSensitivity(Config::CameraSensitivity);

    if (!mTextureManager.Initialize(md3dDevice.Get(), mCommandList.Get())) return false;
    if (!mRenderer.Initialize(md3dDevice.Get())) return false;
    if (!mScene.Initialize(md3dDevice.Get(), mCommandList.Get(), &mTextureManager)) return false;

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, cmdsLists);
    FlushCommandQueue();

    return true;
}

void MainApp::OnResize()
{
    D3DApp::OnResize();
    if (md3dDevice)
    {
        mCamera.SetLens(Config::CameraFOV, AspectRatio(),
            Config::CameraNearZ, Config::CameraFarZ);
    }
}

void MainApp::Update(const GameTimer& gt)
{
    mCamera.HandleInput(gt, mInput);
    mInput.ResetMouseDelta();
    mScene.Update(gt, mCamera);
}

void MainApp::Draw(const GameTimer& gt)
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

void MainApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mInput.OnMouseDown(x, y); SetCapture(mhMainWnd);
}
void MainApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    mInput.OnMouseUp(); ReleaseCapture();
}
void MainApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    mInput.OnMouseMove(x, y, (btnState & MK_LBUTTON) != 0);
}
void MainApp::OnKeyDown(WPARAM keyState, int x, int y)
{
    mInput.OnKeyDown(keyState);
}
void MainApp::OnKeyUp(WPARAM keyState, int x, int y)
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
        MainApp theApp(hInstance);
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