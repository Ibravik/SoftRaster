#pragma once

#include "RePiBase.h"

struct RePiVertex;
struct RePiLine;
struct RePiTriangle;

struct GeometryConstantBuffer
{
    RePiMatrix World;
    RePiMatrix View;
    RePiMatrix Projection;
    RePiMatrix Bones[MaxBoneCapacity];
};

using VertexShader = std::function<RePiVertex(const RePiVertex&, const GeometryConstantBuffer&)>;

class RePiGeometryStage
{
public:
    RePiGeometryStage();

    ~RePiGeometryStage();

    void BindVertexBuffer(
        const std::weak_ptr<std::vector<RePiVertex>>& VertexBuffer);

    void BindIndexBuffer(
        const std::weak_ptr<std::vector<uint32_t>>& IndexBuffer);

    void BindConstantBuffer(
        const std::weak_ptr<GeometryConstantBuffer>& ConstantBuffer);

    void BindVertexShader(
        const VertexShader& Shader);

    void BindTopology(
        RePiVertexTopology Topology);

    void Execute();

    const std::weak_ptr<std::vector<RePiTriangle>> GetTriangleList() const;

    const std::weak_ptr<std::vector<RePiLine>> GetLineList() const;

    const std::weak_ptr<std::vector<RePiVertex>> GetPointList() const;

    const RePiVertexTopology GetTopology() const;

private:
    RepiTriangleOrientation GetTriangleOrientation(
        const RePiTriangle& t) const;

    void ClipVertex(
        RePiVertex& v) const;

private:
    std::weak_ptr<std::vector<RePiVertex>> mVertexBuffer;
    std::weak_ptr<std::vector<uint32_t>> mIndexBuffer;
    std::weak_ptr<GeometryConstantBuffer> mConstantBuffer;
    RePiVertexTopology mTopology;
    VertexShader mShader;

protected:
    std::shared_ptr<std::vector<RePiTriangle>> mTriangleList;
    std::shared_ptr<std::vector<RePiLine>> mLineList;
    std::shared_ptr<std::vector<RePiVertex>> mPointList;
};
