#include "Octree.h"
#include "../Core/Config.h"
#include <algorithm>
#include <cfloat>

using namespace DirectX;

void Octree::Clear()
{
    mRoot.reset();
}

void Octree::Build(const std::vector<GameObject*>& objects)
{
    Clear();
    if (objects.empty())
        return;

    XMFLOAT3 minP(FLT_MAX, FLT_MAX, FLT_MAX);
    XMFLOAT3 maxP(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (GameObject* object : objects)
    {
        const BoundingBox& b = object->GetWorldBounds();
        minP.x = (std::min)(minP.x, b.Center.x - b.Extents.x);
        minP.y = (std::min)(minP.y, b.Center.y - b.Extents.y);
        minP.z = (std::min)(minP.z, b.Center.z - b.Extents.z);
        maxP.x = (std::max)(maxP.x, b.Center.x + b.Extents.x);
        maxP.y = (std::max)(maxP.y, b.Center.y + b.Extents.y);
        maxP.z = (std::max)(maxP.z, b.Center.z + b.Extents.z);
    }

    const float pad = 0.5f;
    minP.x -= pad; minP.y -= pad; minP.z -= pad;
    maxP.x += pad; maxP.y += pad; maxP.z += pad;

    mRoot = std::make_unique<Node>();
    BoundingBox::CreateFromPoints(mRoot->Bounds, XMLoadFloat3(&minP), XMLoadFloat3(&maxP));
    Insert(mRoot.get(), objects, 0);
}

void Octree::Query(const BoundingFrustum& frustum, std::vector<GameObject*>& out) const
{
    out.clear();
    QueryNode(mRoot.get(), frustum, out);
}

void Octree::Subdivide(const BoundingBox& parent, BoundingBox children[8]) const
{
    const XMFLOAT3& c = parent.Center;
    const XMFLOAT3 e = { parent.Extents.x * 0.5f, parent.Extents.y * 0.5f, parent.Extents.z * 0.5f };

    const float ox[2] = { -e.x, e.x };
    const float oy[2] = { -e.y, e.y };
    const float oz[2] = { -e.z, e.z };

    int i = 0;
    for (int x = 0; x < 2; ++x)
    {
        for (int y = 0; y < 2; ++y)
        {
            for (int z = 0; z < 2; ++z)
            {
                children[i].Center = XMFLOAT3(c.x + ox[x], c.y + oy[y], c.z + oz[z]);
                children[i].Extents = e;
                ++i;
            }
        }
    }
}

int Octree::ChildContaining(const BoundingBox& nodeBounds, const BoundingBox& objectBounds) const
{
    BoundingBox children[8];
    Subdivide(nodeBounds, children);

    int found = -1;
    for (int i = 0; i < 8; ++i)
    {
        if (children[i].Contains(objectBounds) == CONTAINS)
        {
            if (found >= 0)
                return -1;
            found = i;
        }
    }
    return found;
}

void Octree::Insert(Node* node, const std::vector<GameObject*>& objects, int depth)
{
    if (!node || objects.empty())
        return;

    if (depth >= Config::OctreeMaxDepth || (int)objects.size() <= Config::OctreeMaxLeafObjects)
    {
        node->Objects = objects;
        return;
    }

    std::vector<GameObject*> buckets[8];
    std::vector<GameObject*> remain;

    for (GameObject* object : objects)
    {
        int child = ChildContaining(node->Bounds, object->GetWorldBounds());
        if (child < 0)
            remain.push_back(object);
        else
            buckets[child].push_back(object);
    }

    node->Objects = std::move(remain);

    BoundingBox childBounds[8];
    Subdivide(node->Bounds, childBounds);

    for (int i = 0; i < 8; ++i)
    {
        if (buckets[i].empty())
            continue;
        node->Children[i] = std::make_unique<Node>();
        node->Children[i]->Bounds = childBounds[i];
        Insert(node->Children[i].get(), buckets[i], depth + 1);
    }
}

void Octree::CollectAll(const Node* node, std::vector<GameObject*>& out) const
{
    if (!node)
        return;
    out.insert(out.end(), node->Objects.begin(), node->Objects.end());
    for (int i = 0; i < 8; ++i)
        CollectAll(node->Children[i].get(), out);
}

void Octree::QueryNode(const Node* node, const BoundingFrustum& frustum, std::vector<GameObject*>& out) const
{
    if (!node)
        return;

    const ContainmentType nodeHit = frustum.Contains(node->Bounds);
    if (nodeHit == DISJOINT)
        return;

    if (nodeHit == CONTAINS)
    {
        CollectAll(node, out);
        return;
    }

    for (GameObject* object : node->Objects)
    {
        if (frustum.Contains(object->GetWorldBounds()) != DISJOINT)
            out.push_back(object);
    }

    for (int i = 0; i < 8; ++i)
        QueryNode(node->Children[i].get(), frustum, out);
}
