#pragma once

#include "../../Common/d3dUtil.h"
#include "../../Common/GameTimer.h"
#include "GameObject.h"
#include <DirectXColors.h>

class BoxObject : public GameObject
{
public:
    BoxObject();
    explicit BoxObject(const std::string& name);
    virtual ~BoxObject() = default;

    virtual bool Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) override;
    virtual void Update(const GameTimer& gt) override;
    virtual void Draw(ID3D12GraphicsCommandList* cmdList, ID3D12DescriptorHeap* cbvHeap) override;

    void SetColor(const DirectX::XMFLOAT4& color) { mColor = color; }
    DirectX::XMFLOAT4 GetColor() const { return mColor; }

private:
    DirectX::XMFLOAT4 mColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    std::shared_ptr<MeshGeometry> mGeometry = nullptr;

    static std::shared_ptr<MeshGeometry> sSharedBoxGeo;
    static std::shared_ptr<MeshGeometry> CreateBoxGeometry(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

    static const std::array<Vertex, 8> sVertices;
    static const std::array<std::uint16_t, 36> sIndices;
};