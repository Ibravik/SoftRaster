#pragma once

#include "RePiBase.h"
#include "RePiMetadata.h"

class RePiMaterial;
class RePiAnimator;

struct RePiVertex
{
public:
    RePiVertex(const RePiFloat3& VertexPosition = RePiFloat3::ZERO,
               const RePiFloat2& VertexTexcoord = RePiFloat2::ZERO,
               const RePiFloat3& VertexNormal = RePiFloat3::ZERO,
               const RePiFloat3& VertexBinormal = RePiFloat3::ZERO,
               const RePiFloat3& VertexTangent = RePiFloat3::ZERO) :
        Position(VertexPosition, 1.f),
        TexCoord(VertexTexcoord),
        Normal(VertexNormal),
        Binormal(VertexBinormal),
        Tangent(VertexTangent),
        BoneWeight(RePiFloat4::ZERO)
    {
        memset(&BoneIndex[0], 0, sizeof(uint32_t) * 4);
    }

public:
    uint32_t BoneIndex[4];

    RePiFloat4 BoneWeight;

    RePiFloat4 Position;

    RePiFloat2 TexCoord;

    RePiFloat3 Normal;

    RePiFloat3 Binormal;

    RePiFloat3 Tangent;
};

struct RePiLine
{
    RePiLine() = default;

    RePiLine(
        const RePiVertex& _v0,
        const RePiVertex& _v1)
        : v0(_v0)
        , v1(_v1)
    {
    };

    RePiVertex v0;
    RePiVertex v1;
};

struct RePiTriangle
{
    RePiTriangle() = default;

    RePiTriangle(
        const RePiVertex& _v0,
        const RePiVertex& _v1,
        const RePiVertex& _v2)
        : v0(_v0)
        , v1(_v1)
        , v2(_v2)
        , orientation(RepiTriangleOrientation::eC)
    {
    };

    RePiVertex v0;
    RePiVertex v1;
    RePiVertex v2;
    RepiTriangleOrientation orientation;
};

class RePiMesh : public RePiMetadata
{
public:
    RePiMesh();

    ~RePiMesh();

    void addVertex(
        const RePiVertex& vertex);

    void addIndex(
        const uint32_t index);

    void setTopology(
        const RePiVertexTopology topology);

    void setMaterial(
        const std::weak_ptr<RePiMaterial>& material);

    std::weak_ptr<std::vector<RePiVertex>> getVertexBuffer() const;

    std::weak_ptr<std::vector<uint32_t>> getIndexBuffer() const;

    std::weak_ptr<RePiMaterial> getMaterial() const;

    RePiVertexTopology getTopology() const;

    //void BindToCommandBuffer(const std::weak_ptr<RePiCommandBuffer>& CommandBuffer) const;

private:
    std::shared_ptr<std::vector<RePiVertex>> m_vertexBuffer;
    std::shared_ptr<std::vector<uint32_t>> m_indexBuffer;
    std::weak_ptr<RePiMaterial> mMaterial;
    RePiVertexTopology m_topology;
};

class RePi3DModel : public RePiMetadata
{
public:
    friend class RePiResourceManager;

    RePi3DModel();

    virtual ~RePi3DModel() = default;

    void AddMesh(
        const std::weak_ptr<RePiMesh>& Mesh);

    void BindAnimator(
        const std::weak_ptr<RePiAnimator>& Animator);

    void SetTransform(
        const RePiMatrix& Transform);

    RePiMatrix GetTransform() const;

    std::weak_ptr<RePiAnimator> GetAnimator() const;

    uint32_t GetMeshCount() const;

    //virtual void BindRenderResourcesToContext(const std::weak_ptr<RePiCommandBuffer>& RenderContext) override;

    void Update();

    std::vector<std::weak_ptr<RePiMesh>> mMeshList;
    RePiMatrix m_boneTransform[MaxBoneCapacity];
protected:
    RePiMatrix m_transform;

    

    std::weak_ptr<RePiAnimator> mAnimator;

   
};
