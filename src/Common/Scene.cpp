#include "Scene.h"
#include "../Main/Box/BoxObject.h"
#include "../Main/Sponza/SponzaObject.h"
#include <algorithm>

using namespace DirectX;

Scene::Scene()
{
}

bool Scene::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    auto sponza = std::make_unique<SponzaObject>("Sponza");
    sponza->SetPosition(0.0f, 0.0f, 0.0f);
    sponza->SetRotation(0.0f, 0.0f, 0.0f);
    sponza->SetScale(1.0f, 1.0f, 1.0f);
    if (!sponza->Initialize(device, cmdList))
        return false;
    AddObject(std::move(sponza));

    auto box = std::make_unique<BoxObject>("GrayCube");
    box->SetPosition(2.0f, 2.0f, 0.0f);
    box->SetRotation(0.0f, 0.0f, 0.0f);
    box->SetScale(0.5f, 0.5f, 0.5f);
    if (!box->Initialize(device, cmdList))
        return false;
    AddObject(std::move(box));

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