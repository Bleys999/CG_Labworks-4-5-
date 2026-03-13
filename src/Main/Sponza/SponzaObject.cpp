#include "SponzaObject.h"
#include <string>
#include <vector>
#include <fstream>

using namespace DirectX;

SponzaObject::SponzaObject() : GameObject("Sponza")
{
}

SponzaObject::SponzaObject(const std::string& name) : GameObject(name)
{
}

bool SponzaObject::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    if (!LoadModel(device, cmdList))
        return false;

    mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, 1, true);
    return true;
}

void SponzaObject::Update(const GameTimer& gt)
{
}

void SponzaObject::UpdateConstantBuffer(DirectX::FXMMATRIX view, DirectX::FXMMATRIX proj)
{
    if (!mVisible) return;

    XMMATRIX world = GetWorldMatrix();
    XMMATRIX worldViewProj = world * view * proj;

    ObjectConstants objConstants;
    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));

    mObjectCB->CopyData(0, objConstants);
}

void SponzaObject::Draw(ID3D12GraphicsCommandList* cmdList)
{
    if (!mGeometry || !mVisible || !mObjectCB) return;

    cmdList->SetGraphicsRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress());

    cmdList->IASetVertexBuffers(0, 1, &mGeometry->VertexBufferView());
    cmdList->IASetIndexBuffer(&mGeometry->IndexBufferView());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->DrawIndexedInstanced(mGeometry->DrawArgs["sponza"].IndexCount, 1, 0, 0, 0);
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

    std::string objPath = std::string(exePath) + "\\models\\sponza.obj";
    std::string mtlPath = std::string(exePath) + "\\models";

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
        objPath.c_str(), mtlPath.c_str(), true);

    if (!ret) {
        MessageBoxA(nullptr, "Failed to load OBJ file", "Error", MB_OK);
        return false;
    }

    std::vector<ObjVertex> vertices;
    std::vector<uint32_t> indices;

    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;

        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            int fv = shapes[s].mesh.num_face_vertices[f];

            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                float vx = attrib.vertices[3 * idx.vertex_index + 0];
                float vy = attrib.vertices[3 * idx.vertex_index + 1];
                float vz = attrib.vertices[3 * idx.vertex_index + 2];

                float nx = 0.0f, ny = 0.0f, nz = 0.0f;
                if (idx.normal_index >= 0) {
                    nx = attrib.normals[3 * idx.normal_index + 0];
                    ny = attrib.normals[3 * idx.normal_index + 1];
                    nz = attrib.normals[3 * idx.normal_index + 2];
                }

                ObjVertex vertex;
                vertex.Pos = XMFLOAT3(vx, vy, vz);
                vertex.Normal = XMFLOAT3(nx, ny, nz);

                vertices.push_back(vertex);
                indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
            }
            index_offset += fv;
        }
    }

    if (vertices.empty()) {
        MessageBoxA(nullptr, "Model has no vertices!", "Error", MB_OK);
        return false;
    }

    mIndexCount = static_cast<UINT>(indices.size());
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
    submesh.IndexCount = mIndexCount;
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    mGeometry->DrawArgs["sponza"] = submesh;

    return true;
}