#pragma once

#include "../../Common/d3dUtil.h"
#include "../../Common/GameTimer.h"
#include "../../Common/tiny_obj_loader.h"
#include "../../Common/GameObject.h"

struct ObjVertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
};

class SponzaObject : public GameObject
{
public:
    SponzaObject();
    explicit SponzaObject(const std::string& name);
    virtual ~SponzaObject() = default;

    virtual bool Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) override;
    virtual void Update(const GameTimer& gt) override;
    virtual void Draw(ID3D12GraphicsCommandList* cmdList) override;
    virtual void UpdateConstantBuffer(DirectX::FXMMATRIX view, DirectX::FXMMATRIX proj) override;

private:
    bool LoadModel(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

private:
    std::unique_ptr<MeshGeometry> mGeometry = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
    UINT mIndexCount = 0;
};