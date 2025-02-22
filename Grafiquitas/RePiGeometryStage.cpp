#include "RePiGeometryStage.h"

#include "RePi3DModel.h"

RePiGeometryStage::RePiGeometryStage()
{
    mPointList = std::make_shared<std::vector<RePiVertex>>();
    mLineList = std::make_shared<std::vector<RePiLine>>();
    mTriangleList = std::make_shared<std::vector<RePiTriangle>>();
}

RePiGeometryStage::~RePiGeometryStage()
{
    mPointList.reset();
    mLineList.reset();
    mTriangleList.reset();
}

void RePiGeometryStage::BindVertexBuffer(
    const std::weak_ptr<std::vector<RePiVertex>>& VertexBuffer)
{
    mVertexBuffer = VertexBuffer;
}

void RePiGeometryStage::BindIndexBuffer(
    const std::weak_ptr<std::vector<uint32_t>>& IndexBuffer)
{
    mIndexBuffer = IndexBuffer;
}

void RePiGeometryStage::BindConstantBuffer(
    const std::weak_ptr<GeometryConstantBuffer>& ConstantBuffer)
{
    mConstantBuffer = ConstantBuffer;
}

void RePiGeometryStage::BindVertexShader(
    const VertexShader& Shader)
{
    mShader = Shader;
}

void RePiGeometryStage::BindTopology(
    RePiVertexTopology Topology)
{
    mTopology = Topology;
}

void RePiGeometryStage::Execute()
{
    if (mShader)
    {
        if (auto pVertexBuffer = mVertexBuffer.lock())
        {
            if (auto pIndexBuffer = mIndexBuffer.lock())
            {
                if (auto pConstantBuffer = mConstantBuffer.lock())
                {
                    mPointList->clear();
                    mLineList->clear();
                    mTriangleList->clear();

                    RePiVertex v0, v1, v2;
                    size_t incremental = size_t(0);

                    if (mTopology == RePiVertexTopology::ePOINTLIST)
                    {
                        mPointList->reserve(size_t(pIndexBuffer->size()));
                        incremental = size_t(1);
                    }
                    else if (mTopology == RePiVertexTopology::eLINELIST)
                    {
                        mLineList->reserve(size_t(pIndexBuffer->size() / 2));
                        incremental = size_t(2);
                    }
                    else if (mTopology == RePiVertexTopology::eTRIANGLELIST)
                    {
                        mTriangleList->reserve(size_t(pIndexBuffer->size() / 3));
                        incremental = size_t(3);
                    }

                    for (size_t i = 0; i < pIndexBuffer->size(); i += incremental)
                    {
                        switch (mTopology)
                        {
                        case ePOINTLIST:
                        {
                            v0 = mShader(pVertexBuffer->at(pIndexBuffer->at(i + 0)), *pConstantBuffer);
                            ClipVertex(v0);

                            mPointList->push_back(v0);
                            break;
                        }
                        case eLINELIST:
                        {
                            v0 = mShader(pVertexBuffer->at(pIndexBuffer->at(i + 0)), *pConstantBuffer);
                            ClipVertex(v0);

                            v1 = mShader(pVertexBuffer->at(pIndexBuffer->at(i + 1)), *pConstantBuffer);
                            ClipVertex(v1);

                            RePiLine line = RePiLine(v0, v1);

                            mLineList->push_back(RePiLine(v0, v1));
                            break;
                        }
                        case eTRIANGLELIST:
                        {
                            v0 = mShader(pVertexBuffer->at(pIndexBuffer->at(i + 0)), *pConstantBuffer);
                            ClipVertex(v0);

                            v1 = mShader(pVertexBuffer->at(pIndexBuffer->at(i + 1)), *pConstantBuffer);
                            ClipVertex(v1);

                            v2 = mShader(pVertexBuffer->at(pIndexBuffer->at(i + 2)), *pConstantBuffer);
                            ClipVertex(v2);

                            RePiTriangle triangle(v0, v1, v2);
                            triangle.orientation = GetTriangleOrientation(triangle);

                            mTriangleList->push_back(triangle);
                            break;
                        }
                        case eTRIANGLESTRIP:
                        case eLINESTRIP:
                        case eUNDEFINED:
                        default:
                            break;
                        }
                    }
                }
            }
        }
    }

    mVertexBuffer.reset();
    mIndexBuffer.reset();
    mTopology = RePiVertexTopology::eUNDEFINED;
}

const std::weak_ptr<std::vector<RePiTriangle>> RePiGeometryStage::GetTriangleList() const
{
    return mTriangleList;
}

const std::weak_ptr<std::vector<RePiLine>> RePiGeometryStage::GetLineList() const
{
    return mLineList;
}

const std::weak_ptr<std::vector<RePiVertex>> RePiGeometryStage::GetPointList() const
{
    return mPointList;
}

const RePiVertexTopology RePiGeometryStage::GetTopology() const
{
    return mTopology;
}

RepiTriangleOrientation RePiGeometryStage::GetTriangleOrientation(
    const RePiTriangle& t) const
{
    float crossProductZ = (t.v1.Position.x - t.v0.Position.x) * (t.v2.Position.y - t.v0.Position.y) - (t.v1.Position.y - t.v0.Position.y) * (t.v2.Position.x - t.v0.Position.x);

    if (crossProductZ > 0)
    {
        return RepiTriangleOrientation::eCW;
    }
    else if (crossProductZ < 0)
    {
        return RepiTriangleOrientation::eCCW;
    }
    else
    {
        return RepiTriangleOrientation::eC;
    }
}

void RePiGeometryStage::ClipVertex(RePiVertex& v) const
{
    v.Position.x /= v.Position.w;
    v.Position.y /= v.Position.w;
    v.Position.z /= v.Position.w;
    v.Position.w /= v.Position.w;
}
