#pragma once

#include "GameObject.h"
#include <memory>
#include <vector>

class Octree
{
public:
    void Clear();
    void Build(const std::vector<GameObject*>& objects);
    void Query(const DirectX::BoundingFrustum& frustum, std::vector<GameObject*>& out) const;

private:
    struct Node
    {
        DirectX::BoundingBox Bounds;
        std::vector<GameObject*> Objects;
        std::unique_ptr<Node> Children[8];
    };

    void Subdivide(const DirectX::BoundingBox& parent, DirectX::BoundingBox children[8]) const;
    void Insert(Node* node, const std::vector<GameObject*>& objects, int depth);
    void QueryNode(const Node* node, const DirectX::BoundingFrustum& frustum, std::vector<GameObject*>& out) const;
    void CollectAll(const Node* node, std::vector<GameObject*>& out) const;
    int ChildContaining(const DirectX::BoundingBox& nodeBounds, const DirectX::BoundingBox& objectBounds) const;

    std::unique_ptr<Node> mRoot;
};
