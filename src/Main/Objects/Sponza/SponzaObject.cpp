#include "SponzaObject.h"
#include <cstring>
#include <string>
#include <vector>

using namespace DirectX;

namespace
{
    std::string JoinUnderModels(const std::string& modelsRoot, const std::string& rel)
    {
        std::string p = rel;
        while (!p.empty() && (p.front() == ' ' || p.front() == '\t'))
            p.erase(p.begin());
        while (!p.empty() && (p.back() == ' ' || p.back() == '\t'))
            p.pop_back();
        if (p.empty())
            return {};
        for (char& c : p)
        {
            if (c == '/')
                c = '\\';
        }
        return modelsRoot + "\\" + p;
    }
}

SponzaObject::SponzaObject() : GameObject("Sponza")
{
}

SponzaObject::SponzaObject(const std::string& name) : GameObject(name)
{
}

bool SponzaObject::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, TextureManager* textureManager)
{
    mTextureManager = textureManager;

    if (!LoadModel(device, cmdList))
        return false;

    mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, 1, true);

    return true;
}

void SponzaObject::Update(const GameTimer& gt)
{
    float t = gt.TotalTime();
    mUVOffset.x = t * Config::TextureScrollSpeedU;
    mUVOffset.y = t * Config::TextureScrollSpeedV;
}

void SponzaObject::UpdateConstantBuffer(DirectX::FXMMATRIX view, DirectX::FXMMATRIX proj)
{
    if (!mVisible) return;

    XMMATRIX world = GetWorldMatrix();
    XMMATRIX worldViewProj = world * view * proj;
    XMStoreFloat4x4(&mWorldViewProj, XMMatrixTranspose(worldViewProj));
    XMStoreFloat4x4(&mWorld, XMMatrixTranspose(world));
}

void SponzaObject::Draw(ID3D12GraphicsCommandList* cmdList)
{
    if (!mGeometry || !mVisible || !mObjectCB) return;

    cmdList->IASetVertexBuffers(0, 1, &mGeometry->VertexBufferView());
    cmdList->IASetIndexBuffer(&mGeometry->IndexBufferView());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (const auto& batch : mMaterialBatches)
    {
        if (batch.IndexCount == 0 || !batch.DiffuseTexture)
            continue;

        ObjectConstants oc{};
        oc.WorldViewProj = mWorldViewProj;
        oc.World = mWorld;
        oc.DiffuseFactor = batch.DiffuseFactor;
        oc.UVScale = mUVScale;
        oc.UVOffset = mUVOffset;
        mObjectCB->CopyData(0, oc);

        cmdList->SetGraphicsRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress());

        ID3D12DescriptorHeap* heap = batch.DiffuseTexture->GetDescriptorHeap();
        cmdList->SetDescriptorHeaps(1, &heap);
        cmdList->SetGraphicsRootDescriptorTable(1, batch.DiffuseTexture->GetSRV());

        cmdList->DrawIndexedInstanced(batch.IndexCount, 1, batch.StartIndexLocation, 0, 0);
    }
}

bool SponzaObject::LoadModel(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    char* lastSlash = strrchr(exePath, '\\');
    if (lastSlash) *lastSlash = 0;

    std::string modelsRoot = std::string(exePath) + "\\models";
    std::string objPath = modelsRoot + "\\sponza.obj";

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
        objPath.c_str(), modelsRoot.c_str(), true);

    if (!ret)
    {
        MessageBoxA(nullptr, "Failed to load OBJ file", "Error", MB_OK);
        return false;
    }

    std::vector<std::pair<std::shared_ptr<Texture>, XMFLOAT4>> matTable;
    if (materials.empty())
    {
        auto white = mTextureManager->GetWhiteTexture();
        if (!white)
            return false;
        matTable.push_back({ white, XMFLOAT4(0.75f, 0.75f, 0.75f, 1.0f) });
    }
    else
    {
        for (const auto& m : materials)
        {
            XMFLOAT4 kd(
                (float)m.diffuse[0],
                (float)m.diffuse[1],
                (float)m.diffuse[2],
                1.0f);

            if (!m.diffuse_texname.empty())
            {
                std::string path = JoinUnderModels(modelsRoot, m.diffuse_texname);
                auto tex = mTextureManager->LoadTexture(path);
                if (tex)
                    matTable.push_back({ tex, kd });
                else
                {
                    auto white = mTextureManager->GetWhiteTexture();
                    if (!white)
                        return false;
                    matTable.push_back({ white, kd });
                }
            }
            else
            {
                auto white = mTextureManager->GetWhiteTexture();
                if (!white)
                    return false;
                matTable.push_back({ white, kd });
            }
        }
    }

    std::vector<ObjVertex> vertices;
    std::vector<uint32_t> indices;
    mMaterialBatches.clear();

    bool hasTexCoords = !attrib.texcoords.empty();

    int activeMat = -1;
    UINT rangeStart = 0;

    auto flushRange = [&](UINT rangeEnd)
    {
        if (rangeEnd <= rangeStart || activeMat < 0)
            return;
        int mid = activeMat;
        if (mid >= (int)matTable.size())
            mid = 0;

        MaterialDrawBatch batch;
        batch.StartIndexLocation = rangeStart;
        batch.IndexCount = rangeEnd - rangeStart;
        batch.DiffuseTexture = matTable[mid].first;
        batch.DiffuseFactor = matTable[mid].second;
        mMaterialBatches.push_back(batch);
        rangeStart = rangeEnd;
    };

    for (size_t s = 0; s < shapes.size(); s++)
    {
        size_t index_offset = 0;
        const auto& midVec = shapes[s].mesh.material_ids;

        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            int fv = shapes[s].mesh.num_face_vertices[f];

            int mid = 0;
            if (f < midVec.size())
                mid = midVec[f];
            if (mid < 0 || mid >= (int)matTable.size())
                mid = 0;

            UINT idxBefore = (UINT)indices.size();

            if (activeMat < 0)
            {
                activeMat = mid;
                rangeStart = idxBefore;
            }
            else if (mid != activeMat)
            {
                flushRange(idxBefore);
                activeMat = mid;
                rangeStart = idxBefore;
            }

            for (size_t v = 0; v < (size_t)fv; v++)
            {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                float vx = attrib.vertices[3 * idx.vertex_index + 0];
                float vy = attrib.vertices[3 * idx.vertex_index + 1];
                float vz = attrib.vertices[3 * idx.vertex_index + 2];

                float nx = 0.0f, ny = 0.0f, nz = 0.0f;
                if (idx.normal_index >= 0)
                {
                    nx = attrib.normals[3 * idx.normal_index + 0];
                    ny = attrib.normals[3 * idx.normal_index + 1];
                    nz = attrib.normals[3 * idx.normal_index + 2];
                }

                float tx = 0.0f, ty = 0.0f;
                if (hasTexCoords && idx.texcoord_index >= 0)
                {
                    tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                    ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                }

                ObjVertex vertex;
                vertex.Pos = XMFLOAT3(vx, vy, vz);
                vertex.Normal = XMFLOAT3(nx, ny, nz);
                vertex.TexC = XMFLOAT2(tx, ty);

                vertices.push_back(vertex);
                indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
            }
            index_offset += (size_t)fv;
        }
    }

    flushRange((UINT)indices.size());

    if (vertices.empty())
    {
        MessageBoxA(nullptr, "Model has no vertices!", "Error", MB_OK);
        return false;
    }

    const UINT vbByteSize = static_cast<UINT>(vertices.size()) * sizeof(ObjVertex);
    const UINT ibByteSize = static_cast<UINT>(indices.size()) * sizeof(uint32_t);

    mGeometry = std::make_unique<MeshGeometry>();
    mGeometry->Name = "sponza";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &mGeometry->VertexBufferCPU));
    CopyMemory(mGeometry->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &mGeometry->IndexBufferCPU));
    CopyMemory(mGeometry->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    mGeometry->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
        cmdList, vertices.data(), vbByteSize, mGeometry->VertexBufferUploader);

    mGeometry->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
        cmdList, indices.data(), ibByteSize, mGeometry->IndexBufferUploader);

    mGeometry->VertexByteStride = sizeof(ObjVertex);
    mGeometry->VertexBufferByteSize = vbByteSize;
    mGeometry->IndexFormat = DXGI_FORMAT_R32_UINT;
    mGeometry->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = static_cast<UINT>(indices.size());
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    mGeometry->DrawArgs["sponza"] = submesh;

    return true;
}
