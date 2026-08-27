#pragma once

#include "GBuffer.h"
#include "../D3D12/d3dUtil.h"
#include "../Scene/Camera.h"
#include <memory>
#include <vector>

static constexpr UINT kMaxPointLights = 8;
static constexpr UINT kMaxSpotLights = 4;

struct DeferredLightingConstants
{
    DirectX::XMFLOAT4 DirectionalDir = { 0.35f, -0.85f, 0.35f, 0.0f };
    DirectX::XMFLOAT4 DirectionalColor = { 1.0f, 0.95f, 0.85f, 0.55f };
    DirectX::XMFLOAT4 AmbientColor = { 0.06f, 0.06f, 0.07f, 1.0f };
    DirectX::XMFLOAT4 CameraWorld = { 0.0f, 0.0f, 0.0f, 0.0f };
    UINT PointCount = 0;
    UINT SpotCount = 0;
    UINT Pad[2] = {};
    DirectX::XMFLOAT4 PointPosRange[kMaxPointLights] = {};
    DirectX::XMFLOAT4 PointColorIntensity[kMaxPointLights] = {};
    DirectX::XMFLOAT4 SpotPosRange[kMaxSpotLights] = {};
    DirectX::XMFLOAT4 SpotDirCosine[kMaxSpotLights] = {};
    DirectX::XMFLOAT4 SpotColorIntensity[kMaxSpotLights] = {};
};

class RenderingSystem
{
public:
    bool Initialize(ID3D12Device* device);
    void Resize(ID3D12Device* device, UINT width, UINT height);

    GBuffer& GetGBuffer() { return mGBuffer; }

    void ApplyGeometryPass(ID3D12GraphicsCommandList* cmdList) const;
    ID3D12PipelineState* GetGeometryPSO() const { return mGeometryPSO.Get(); }

    void UpdateLightingConstants(const Camera& camera);
    void DrawDeferredLighting(
        ID3D12GraphicsCommandList* cmdList,
        D3D12_CPU_DESCRIPTOR_HANDLE backBufferRtv,
        const D3D12_VIEWPORT& viewport,
        const D3D12_RECT& scissor) const;

private:
    void BuildGeometryPass(ID3D12Device* device);
    void BuildLightingPass(ID3D12Device* device);
    void InitDefaultLights();

private:
    GBuffer mGBuffer;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> mGeometryRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mGeometryPSO;
    Microsoft::WRL::ComPtr<ID3DBlob> mGeometryVs;
    Microsoft::WRL::ComPtr<ID3DBlob> mGeometryPs;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mGeometryInputLayout;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> mLightingRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mLightingPSO;
    Microsoft::WRL::ComPtr<ID3DBlob> mLightingVs;
    Microsoft::WRL::ComPtr<ID3DBlob> mLightingPs;

    std::unique_ptr<UploadBuffer<DeferredLightingConstants>> mLightingCB;

    DeferredLightingConstants mLightingCpu = {};
};
