#pragma once

#include "../D3D12/d3dUtil.h"
#include "../Core/GameTimer.h"
#include <string>

class TextureManager;

class GameObject
{
public:
    GameObject() = default;
    explicit GameObject(const std::string& name) : mName(name) {}
    virtual ~GameObject() = default;

    virtual void Update(const GameTimer& gt) = 0;
    virtual void Draw(ID3D12GraphicsCommandList* cmdList) = 0;
    virtual void UpdateConstantBuffer(DirectX::FXMMATRIX view, DirectX::FXMMATRIX proj) = 0;

    virtual bool Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) { return true; }
    virtual bool Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, TextureManager* textureManager) { return true; }

    void SetPosition(float x, float y, float z) { mPosition = DirectX::XMFLOAT3(x, y, z); }
    void SetPosition(const DirectX::XMFLOAT3& pos) { mPosition = pos; }
    DirectX::XMFLOAT3 GetPosition() const { return mPosition; }

    void SetRotation(float x, float y, float z) { mRotation = DirectX::XMFLOAT3(x, y, z); }
    void SetRotation(const DirectX::XMFLOAT3& rot) { mRotation = rot; }
    DirectX::XMFLOAT3 GetRotation() const { return mRotation; }

    void SetScale(float x, float y, float z) { mScale = DirectX::XMFLOAT3(x, y, z); }
    void SetScale(const DirectX::XMFLOAT3& scale) { mScale = scale; }
    DirectX::XMFLOAT3 GetScale() const { return mScale; }

    void SetName(const std::string& name) { mName = name; }
    const std::string& GetName() const { return mName; }

    void SetVisible(bool visible) { mVisible = visible; }
    bool IsVisible() const { return mVisible; }

protected:
    DirectX::XMMATRIX GetWorldMatrix() const
    {
        DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
        DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(mRotation.x, mRotation.y, mRotation.z);
        DirectX::XMMATRIX scaling = DirectX::XMMatrixScaling(mScale.x, mScale.y, mScale.z);
        return scaling * rotation * translation;
    }

protected:
    DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 mRotation = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 mScale = { 1.0f, 1.0f, 1.0f };
    std::string mName;
    bool mVisible = true;
};