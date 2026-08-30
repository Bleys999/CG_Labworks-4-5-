#pragma once

#include "../../../Common/D3D12/d3dUtil.h"
#include "../../../Common/Core/GameTimer.h"
#include "../../../Common/Scene/GameObject.h"
#include "../../../Common/Assets/TextureManager.h"
#include "../../../Common/Assets/Texture.h"
#include <memory>

class TessellatedObject : public GameObject
{
public:
    TessellatedObject();
    explicit TessellatedObject(const std::string& name);
    virtual ~TessellatedObject() = default;

    virtual bool Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, TextureManager* textureManager) override;
    virtual void Update(const GameTimer& gt) override;
    virtual void Draw(ID3D12GraphicsCommandList* cmdList) override;
    virtual void UpdateConstantBuffer(DirectX::FXMMATRIX view, DirectX::FXMMATRIX proj) override;
    virtual bool UsesTessellation() const override { return true; }
    virtual void SetFrameCamera(const DirectX::XMFLOAT3& pos) override;

private:
    bool CreateSharedResources(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, TextureManager* textureManager);
    bool CreateMaterialHeap(ID3D12Device* device);

private:
    std::shared_ptr<MeshGeometry> mGeometry;
    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB;
    DirectX::XMFLOAT3 mCameraWorld = { 0.0f, 0.0f, 0.0f };

    static std::shared_ptr<MeshGeometry> sSharedGeometry;
    static std::shared_ptr<Texture> sAlbedo;
    static std::shared_ptr<Texture> sNormal;
    static std::shared_ptr<Texture> sDisplacement;
    static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> sSrvHeap;
};
