#pragma once

#include "d3dUtil.h"
#include <vector>

class Renderer
{
public:
    Renderer() = default;
    ~Renderer() = default;

    bool Initialize(ID3D12Device* device);
    void Apply(ID3D12GraphicsCommandList* cmdList);
    ID3D12PipelineState* GetPSO() const { return mPSO.Get(); }

private:
    void BuildRootSignature(ID3D12Device* device);
    void BuildShadersAndInputLayout();
    void BuildPSO(ID3D12Device* device);

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO;
    Microsoft::WRL::ComPtr<ID3DBlob> mvsByteCode;
    Microsoft::WRL::ComPtr<ID3DBlob> mpsByteCode;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
};