#include "BoxObject.h"

using namespace DirectX;

const std::array<Vertex, 8> BoxObject::sVertices = { {
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f) },
    { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f) },
    { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f) },
    { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f) },
    { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f) },
    { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f) },
    { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f) },
    { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f) }
} };

const std::array<std::uint16_t, 36> BoxObject::sIndices = { {
    0, 1, 2, 0, 2, 3,
    4, 6, 5, 4, 7, 6,
    4, 5, 1, 4, 1, 0,
    3, 2, 6, 3, 6, 7,
    1, 5, 6, 1, 6, 2,
    4, 0, 3, 4, 3, 7
} };

BoxObject::BoxObject() : GameObject("Box")
{
}

BoxObject::BoxObject(const std::string& name) : GameObject(name)
{
}

bool BoxObject::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    if (!CreateGeometry(device, cmdList))
        return false;

    mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, 1, true);
    return true;
}

void BoxObject::Update(const GameTimer& gt)
{
}

void BoxObject::UpdateConstantBuffer(DirectX::FXMMATRIX view, DirectX::FXMMATRIX proj)
{
    if (!mVisible) return;

    XMMATRIX world = GetWorldMatrix();
    XMMATRIX worldViewProj = world * view * proj;

    ObjectConstants objConstants{};
    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
    objConstants.DiffuseFactor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    objConstants.UVScale = XMFLOAT2(1.0f, 1.0f);
    objConstants.UVOffset = XMFLOAT2(0.0f, 0.0f);

    mObjectCB->CopyData(0, objConstants);
}

void BoxObject::Draw(ID3D12GraphicsCommandList* cmdList)
{
    if (!mGeometry || !mVisible || !mObjectCB) return;

    cmdList->SetGraphicsRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress());

    cmdList->IASetVertexBuffers(0, 1, &mGeometry->VertexBufferView());
    cmdList->IASetIndexBuffer(&mGeometry->IndexBufferView());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->DrawIndexedInstanced(mGeometry->DrawArgs["box"].IndexCount, 1, 0, 0, 0);
}

bool BoxObject::CreateGeometry(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    mGeometry = std::make_unique<MeshGeometry>();
    mGeometry->Name = "boxGeo";

    const UINT vbByteSize = (UINT)sVertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)sIndices.size() * sizeof(std::uint16_t);

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &mGeometry->VertexBufferCPU));
    CopyMemory(mGeometry->VertexBufferCPU->GetBufferPointer(), sVertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &mGeometry->IndexBufferCPU));
    CopyMemory(mGeometry->IndexBufferCPU->GetBufferPointer(), sIndices.data(), ibByteSize);

    mGeometry->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
        cmdList, sVertices.data(), vbByteSize, mGeometry->VertexBufferUploader);

    mGeometry->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
        cmdList, sIndices.data(), ibByteSize, mGeometry->IndexBufferUploader);

    mGeometry->VertexByteStride = sizeof(Vertex);
    mGeometry->VertexBufferByteSize = vbByteSize;
    mGeometry->IndexFormat = DXGI_FORMAT_R16_UINT;
    mGeometry->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)sIndices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    mGeometry->DrawArgs["box"] = submesh;

    return true;
}