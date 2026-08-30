#include "RenderingSystem.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
    UINT CompileFlags()
    {
        UINT f = 0;
#if defined(DEBUG) || defined(_DEBUG)
        f |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        return f;
    }
}

bool RenderingSystem::Initialize(ID3D12Device* device)
{
    mLightingCB = std::make_unique<UploadBuffer<DeferredLightingConstants>>(device, 1, true);
    InitDefaultLights();
    BuildGeometryPass(device);
    BuildTessellationPass(device);
    BuildLightingPass(device);
    return true;
}

void RenderingSystem::Resize(ID3D12Device* device, UINT width, UINT height)
{
    mGBuffer.Resize(device, width, height);
}

void RenderingSystem::InitDefaultLights()
{
    mLightingCpu.DirectionalDir = { 0.2f, -1.0f, 0.15f, 0.0f };
    mLightingCpu.DirectionalColor = { 1.0f, 0.96f, 0.88f, 0.45f };
    mLightingCpu.AmbientColor = { 0.08f, 0.08f, 0.09f, 1.0f };

    mLightingCpu.PointCount = 6;
    mLightingCpu.PointPosRange[0] = { -5.0f, 6.5f, 0.0f, 22.0f };
    mLightingCpu.PointColorIntensity[0] = { 1.0f, 0.15f, 0.1f, 5.5f };
    mLightingCpu.PointPosRange[1] = { 5.0f, 6.5f, 0.0f, 22.0f };
    mLightingCpu.PointColorIntensity[1] = { 0.12f, 0.35f, 1.0f, 5.5f };
    mLightingCpu.PointPosRange[2] = { 0.0f, 6.5f, 5.0f, 22.0f };
    mLightingCpu.PointColorIntensity[2] = { 0.12f, 1.0f, 0.22f, 5.0f };
    mLightingCpu.PointPosRange[3] = { 0.0f, 6.5f, -5.0f, 22.0f };
    mLightingCpu.PointColorIntensity[3] = { 1.0f, 0.85f, 0.12f, 5.0f };
    mLightingCpu.PointPosRange[4] = { 0.0f, 7.5f, 0.0f, 18.0f };
    mLightingCpu.PointColorIntensity[4] = { 1.0f, 0.15f, 0.85f, 4.5f };
    mLightingCpu.PointPosRange[5] = { -3.5f, 6.0f, -3.5f, 20.0f };
    mLightingCpu.PointColorIntensity[5] = { 0.15f, 0.95f, 1.0f, 4.5f };

    mLightingCpu.SpotCount = 2;
    mLightingCpu.SpotPosRange[0] = { -2.0f, 11.0f, -2.0f, 35.0f };
    mLightingCpu.SpotDirCosine[0] = { 0.15f, -1.0f, 0.25f, 0.72f };
    mLightingCpu.SpotColorIntensity[0] = { 0.35f, 0.7f, 1.0f, 4.0f };
    mLightingCpu.SpotPosRange[1] = { 3.0f, 11.0f, 2.0f, 35.0f };
    mLightingCpu.SpotDirCosine[1] = { -0.2f, -1.0f, -0.15f, 0.72f };
    mLightingCpu.SpotColorIntensity[1] = { 1.0f, 0.55f, 0.15f, 4.0f };
}

void RenderingSystem::UpdateLightingConstants(const Camera& camera)
{
    XMVECTOR eye = camera.GetPosition();
    XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&mLightingCpu.CameraWorld), eye);
    mLightingCB->CopyData(0, mLightingCpu);
}

void RenderingSystem::ApplyGeometryPass(ID3D12GraphicsCommandList* cmdList) const
{
    cmdList->SetGraphicsRootSignature(mGeometryRootSignature.Get());
}

void RenderingSystem::ApplyTessellationPass(ID3D12GraphicsCommandList* cmdList) const
{
    cmdList->SetGraphicsRootSignature(mTessellationRootSignature.Get());
    cmdList->SetPipelineState(mTessellationPSO.Get());
}

void RenderingSystem::DrawDeferredLighting(
    ID3D12GraphicsCommandList* cmdList,
    D3D12_CPU_DESCRIPTOR_HANDLE backBufferRtv,
    const D3D12_VIEWPORT& viewport,
    const D3D12_RECT& scissor) const
{
    cmdList->SetGraphicsRootSignature(mLightingRootSignature.Get());
    cmdList->SetPipelineState(mLightingPSO.Get());

    ID3D12DescriptorHeap* heaps[] = { mGBuffer.SrvHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetGraphicsRootConstantBufferView(0, mLightingCB->Resource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(1, mGBuffer.SrvGpuStart());

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cmdList->OMSetRenderTargets(1, &backBufferRtv, false, nullptr);
    cmdList->ClearRenderTargetView(backBufferRtv, black, 0, nullptr);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);
}

void RenderingSystem::BuildGeometryPass(ID3D12Device* device)
{
    ComPtr<ID3DBlob> errors;

    HRESULT hr = D3DCompileFromFile(L"Shaders\\gbuffer.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VS", "vs_5_0", CompileFlags(), 0, &mGeometryVs, &errors);
    if (FAILED(hr))
    {
        if (errors)
            OutputDebugStringA((char*)errors->GetBufferPointer());
        ThrowIfFailed(hr);
    }

    hr = D3DCompileFromFile(L"Shaders\\gbuffer.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PS", "ps_5_0", CompileFlags(), 0, &mGeometryPs, &errors);
    if (FAILED(hr))
    {
        if (errors)
            OutputDebugStringA((char*)errors->GetBufferPointer());
        ThrowIfFailed(hr);
    }

    CD3DX12_ROOT_PARAMETER slotRootParameter[2];
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsDescriptorTable(1, &CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0));

    CD3DX12_STATIC_SAMPLER_DESC samplerDesc(
        0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        0.0f,
        1,
        D3D12_COMPARISON_FUNC_ALWAYS,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
        0.0f,
        D3D12_FLOAT32_MAX,
        D3D12_SHADER_VISIBILITY_PIXEL,
        0);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 1, &samplerDesc,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSig;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()));
    ThrowIfFailed(device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&mGeometryRootSignature)));

    mGeometryInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { mGeometryInputLayout.data(), (UINT)mGeometryInputLayout.size() };
    psoDesc.pRootSignature = mGeometryRootSignature.Get();
    psoDesc.VS = { reinterpret_cast<BYTE*>(mGeometryVs->GetBufferPointer()), mGeometryVs->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<BYTE*>(mGeometryPs->GetBufferPointer()), mGeometryPs->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = GBuffer::ColorTargetCount;
    psoDesc.RTVFormats[0] = GBuffer::AlbedoFormat;
    psoDesc.RTVFormats[1] = GBuffer::NormalFormat;
    psoDesc.RTVFormats[2] = GBuffer::WorldPosFormat;
    psoDesc.DSVFormat = GBuffer::DepthFormat;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mGeometryPSO)));
}

void RenderingSystem::BuildTessellationPass(ID3D12Device* device)
{
    ComPtr<ID3DBlob> errors;

    auto Compile = [&](const char* entry, const char* target, ComPtr<ID3DBlob>& out)
    {
        errors.Reset();
        HRESULT hr = D3DCompileFromFile(L"Shaders\\tessellation.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entry, target, CompileFlags(), 0, &out, &errors);
        if (FAILED(hr))
        {
            if (errors)
                OutputDebugStringA((char*)errors->GetBufferPointer());
            ThrowIfFailed(hr);
        }
    };

    Compile("VS", "vs_5_0", mTessVs);
    Compile("HS", "hs_5_0", mTessHs);
    Compile("DS", "ds_5_0", mTessDs);
    Compile("PS", "ps_5_0", mTessPs);

    CD3DX12_DESCRIPTOR_RANGE srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);

    CD3DX12_ROOT_PARAMETER slotRootParameter[2];
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsDescriptorTable(1, &srvRange);

    CD3DX12_STATIC_SAMPLER_DESC samplerDesc(
        0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        0.0f,
        1,
        D3D12_COMPARISON_FUNC_ALWAYS,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
        0.0f,
        D3D12_FLOAT32_MAX,
        D3D12_SHADER_VISIBILITY_ALL,
        0);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 1, &samplerDesc,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSig;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()));
    ThrowIfFailed(device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&mTessellationRootSignature)));

    mTessellationInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { mTessellationInputLayout.data(), (UINT)mTessellationInputLayout.size() };
    psoDesc.pRootSignature = mTessellationRootSignature.Get();
    psoDesc.VS = { reinterpret_cast<BYTE*>(mTessVs->GetBufferPointer()), mTessVs->GetBufferSize() };
    psoDesc.HS = { reinterpret_cast<BYTE*>(mTessHs->GetBufferPointer()), mTessHs->GetBufferSize() };
    psoDesc.DS = { reinterpret_cast<BYTE*>(mTessDs->GetBufferPointer()), mTessDs->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<BYTE*>(mTessPs->GetBufferPointer()), mTessPs->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    psoDesc.NumRenderTargets = GBuffer::ColorTargetCount;
    psoDesc.RTVFormats[0] = GBuffer::AlbedoFormat;
    psoDesc.RTVFormats[1] = GBuffer::NormalFormat;
    psoDesc.RTVFormats[2] = GBuffer::WorldPosFormat;
    psoDesc.DSVFormat = GBuffer::DepthFormat;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mTessellationPSO)));
}

void RenderingSystem::BuildLightingPass(ID3D12Device* device)
{
    ComPtr<ID3DBlob> errors;

    HRESULT hr = D3DCompileFromFile(L"Shaders\\deferred_light.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VS", "vs_5_0", CompileFlags(), 0, &mLightingVs, &errors);
    if (FAILED(hr))
    {
        if (errors)
            OutputDebugStringA((char*)errors->GetBufferPointer());
        ThrowIfFailed(hr);
    }

    hr = D3DCompileFromFile(L"Shaders\\deferred_light.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PS", "ps_5_0", CompileFlags(), 0, &mLightingPs, &errors);
    if (FAILED(hr))
    {
        if (errors)
            OutputDebugStringA((char*)errors->GetBufferPointer());
        ThrowIfFailed(hr);
    }

    CD3DX12_DESCRIPTOR_RANGE srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);

    CD3DX12_ROOT_PARAMETER slotRootParameter[2];
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsDescriptorTable(1, &srvRange);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> serializedRootSig;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()));
    ThrowIfFailed(device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&mLightingRootSignature)));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.pRootSignature = mLightingRootSignature.Get();
    psoDesc.VS = { reinterpret_cast<BYTE*>(mLightingVs->GetBufferPointer()), mLightingVs->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<BYTE*>(mLightingPs->GetBufferPointer()), mLightingPs->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = false;
    psoDesc.DepthStencilState.StencilEnable = false;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mLightingPSO)));
}
