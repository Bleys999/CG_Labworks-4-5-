#pragma once

#include "../../../Common/D3D12/d3dUtil.h"
#include "../../../Common/Core/GameTimer.h"
#include "../../../Common/Scene/GameObject.h"

class BoxObject : public GameObject
{
public:
    BoxObject();
    explicit BoxObject(const std::string& name);
    virtual ~BoxObject() = default;

    virtual bool Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) override;
    virtual void Update(const GameTimer& gt) override;
    virtual void Draw(ID3D12GraphicsCommandList* cmdList) override;
    virtual void UpdateConstantBuffer(DirectX::FXMMATRIX view, DirectX::FXMMATRIX proj) override;

private:
    bool CreateGeometry(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

private:
    std::unique_ptr<MeshGeometry> mGeometry = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

    static const std::array<Vertex, 8> sVertices;
    static const std::array<std::uint16_t, 36> sIndices;
};