#pragma once

#include "../../../Common/D3D12/d3dUtil.h"
#include "../../../Common/Core/GameTimer.h"
#include "../../../Common/Assets/tiny_obj_loader.h"
#include "../../../Common/Scene/GameObject.h"
#include "../../../Common/Assets/TextureManager.h"
#include "../../../Common/Assets/Texture.h"
#include "../../../Common/Assets/MathHelper.h"
#include "../../../Common/Core/Config.h"
#include <memory>
#include <vector>

struct ObjVertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexC;
};

class SponzaObject : public GameObject
{
public:
    SponzaObject();
    explicit SponzaObject(const std::string& name);
    virtual ~SponzaObject() = default;

    virtual bool Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, TextureManager* textureManager);
    virtual void Update(const GameTimer& gt) override;
    virtual void Draw(ID3D12GraphicsCommandList* cmdList) override;
    virtual void UpdateConstantBuffer(DirectX::FXMMATRIX view, DirectX::FXMMATRIX proj) override;

private:
    struct MaterialDrawBatch
    {
        UINT StartIndexLocation = 0;
        UINT IndexCount = 0;
        std::shared_ptr<Texture> DiffuseTexture;
        DirectX::XMFLOAT4 DiffuseFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    bool LoadModel(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

private:
    std::unique_ptr<MeshGeometry> mGeometry = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
    TextureManager* mTextureManager = nullptr;
    std::vector<MaterialDrawBatch> mMaterialBatches;
    DirectX::XMFLOAT4X4 mWorldViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT2 mUVScale = { Config::TextureTileU, Config::TextureTileV };
    DirectX::XMFLOAT2 mUVOffset = { 0.0f, 0.0f };
};
