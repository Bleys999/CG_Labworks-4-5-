#include "Scene.h"
#include "../../Main/Objects/Box/BoxObject.h"
#include "../../Main/Objects/Sponza/SponzaObject.h"
#include "../Assets/TextureManager.h"
#include <algorithm>

using namespace DirectX;

Scene::Scene()
    : mTextureManager(nullptr)
{
}

bool Scene::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, TextureManager* textureManager)
{
    mTextureManager = textureManager;

    auto sponza = std::make_unique<SponzaObject>("Sponza");
    sponza->SetPosition(0.0f, 0.0f, 0.0f);
    sponza->SetRotation(0.0f, 0.0f, 0.0f);
    sponza->SetScale(1.0f, 1.0f, 1.0f);
    if (!sponza->Initialize(device, cmdList, textureManager))
        return false;
    AddObject(std::move(sponza));

    return true;
}

void Scene::Update(const GameTimer& gt, const Camera& camera)
{
    XMMATRIX view = camera.GetView();
    XMMATRIX proj = camera.GetProj();

    for (auto& gameObject : mGameObjects)
    {
        gameObject->Update(gt);
        gameObject->UpdateConstantBuffer(view, proj);
    }
}

void Scene::Draw(ID3D12GraphicsCommandList* cmdList)
{
    for (auto& gameObject : mGameObjects)
    {
        gameObject->Draw(cmdList);
    }
}

void Scene::AddObject(std::unique_ptr<GameObject> object)
{
    if (object)
    {
        mGameObjects.push_back(std::move(object));
    }
}

void Scene::RemoveObject(const std::string& name)
{
    auto it = std::find_if(mGameObjects.begin(), mGameObjects.end(),
        [&name](const std::unique_ptr<GameObject>& obj) {
            return obj->GetName() == name;
        });

    if (it != mGameObjects.end())
    {
        mGameObjects.erase(it);
    }
}

GameObject* Scene::GetObject(const std::string& name)
{
    auto it = std::find_if(mGameObjects.begin(), mGameObjects.end(),
        [&name](const std::unique_ptr<GameObject>& obj) {
            return obj->GetName() == name;
        });

    if (it != mGameObjects.end())
    {
        return it->get();
    }
    return nullptr;
}