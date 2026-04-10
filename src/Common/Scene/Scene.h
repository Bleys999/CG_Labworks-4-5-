#pragma once

#include "../D3D12/d3dUtil.h"
#include "../Core/GameTimer.h"
#include "Camera.h"
#include "GameObject.h"
#include <vector>
#include <memory>

class TextureManager;

class Scene
{
public:
    Scene();
    ~Scene() = default;

    bool Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, TextureManager* textureManager);
    void Update(const GameTimer& gt, const Camera& camera);
    void Draw(ID3D12GraphicsCommandList* cmdList);

    void AddObject(std::unique_ptr<GameObject> object);
    void RemoveObject(const std::string& name);
    GameObject* GetObject(const std::string& name);

    size_t GetObjectCount() const { return mGameObjects.size(); }

private:
    std::vector<std::unique_ptr<GameObject>> mGameObjects;
    TextureManager* mTextureManager;
};