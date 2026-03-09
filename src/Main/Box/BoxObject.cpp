#include "BoxObject.h"

using namespace DirectX;

std::shared_ptr<MeshGeometry> BoxObject::sSharedBoxGeo = nullptr;

const std::array<Vertex, 8> BoxObject::sVertices = { {
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) },
    { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) },
    { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) },
    { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) },
    { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) },
    { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) },
    { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) },
    { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) }
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
    if (!sSharedBoxGeo)
    {
        sSharedBoxGeo = CreateBoxGeometry(device, cmdList);
    }
    mGeometry = sSharedBoxGeo;
    return true;
}

void BoxObject::Update(const GameTimer& gt)
{
}

void BoxObject::Draw(ID3D12GraphicsCommandList* cmdList, ID3D12DescriptorHeap* cbvHeap)
{
    if (!mGeometry || !mVisible) return;

    ID3D12Device* device = nullptr;
    cmdList->GetDevice(IID_PPV_ARGS(&device));
    UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->Release();

    D3D12_GPU_DESCRIPTOR_HANDLE cbvHandle = cbvHeap->GetGPUDescriptorHandleForHeapStart();
    cbvHandle.ptr += mCBIndex * descriptorSize;

    cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);

    cmdList->IASetVertexBuffers(0, 1, &mGeometry->VertexBufferView());
    cmdList->IASetIndexBuffer(&mGeometry->IndexBufferView());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->DrawIndexedInstanced(
        mGeometry->DrawArgs["box"].IndexCount,
        1, 0, 0, 0);
}

std::shared_ptr<MeshGeometry> BoxObject::CreateBoxGeometry(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    auto geometry = std::make_shared<MeshGeometry>();
    geometry->Name = "boxGeo";

    const UINT vbByteSize = (UINT)sVertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)sIndices.size() * sizeof(std::uint16_t);

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geometry->VertexBufferCPU));
    CopyMemory(geometry->VertexBufferCPU->GetBufferPointer(), sVertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geometry->IndexBufferCPU));
    CopyMemory(geometry->IndexBufferCPU->GetBufferPointer(), sIndices.data(), ibByteSize);

    geometry->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
        cmdList, sVertices.data(), vbByteSize, geometry->VertexBufferUploader);

    geometry->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device,
        cmdList, sIndices.data(), ibByteSize, geometry->IndexBufferUploader);

    geometry->VertexByteStride = sizeof(Vertex);
    geometry->VertexBufferByteSize = vbByteSize;
    geometry->IndexFormat = DXGI_FORMAT_R16_UINT;
    geometry->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)sIndices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geometry->DrawArgs["box"] = submesh;

    return geometry;
}