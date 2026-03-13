#pragma once

#include "d3dUtil.h"
#include "GameTimer.h"
#include "Camera.h"
#include "GameObject.h"
#include <vector>
#include <memory>

class Scene
{
public:
    Scene();
    ~Scene() = default;

    bool Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
    void Update(const GameTimer& gt, const Camera& camera);
    void Draw(ID3D12GraphicsCommandList* cmdList);

    void AddObject(std::unique_ptr<GameObject> object);
    void RemoveObject(const std::string& name);
    GameObject* GetObject(const std::string& name);

    size_t GetObjectCount() const { return mGameObjects.size(); }

private:
    std::vector<std::unique_ptr<GameObject>> mGameObjects;
};