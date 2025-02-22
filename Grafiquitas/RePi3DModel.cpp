#include "RePi3DModel.h"

#include "RePiAnimator.h"
#include "RePiMaterial.h"

RePiMesh::RePiMesh()
    : m_topology(RePiVertexTopology::eUNDEFINED)
{
    m_vertexBuffer = std::make_shared<std::vector<RePiVertex>>();

    m_indexBuffer = std::make_shared<std::vector<uint32_t>>();
}

RePiMesh::~RePiMesh()
{
    m_vertexBuffer.reset();

    m_indexBuffer.reset();
}

void RePiMesh::addVertex(
    const RePiVertex& vertex)
{
    m_vertexBuffer->push_back(vertex);
}

void RePiMesh::addIndex(
    const uint32_t index)
{
    m_indexBuffer->push_back(index);
}

void RePiMesh::setTopology(
    const RePiVertexTopology topology)
{
    m_topology = topology;
}

void RePiMesh::setMaterial(
    const std::weak_ptr<RePiMaterial>& material)
{
    mMaterial = material;
}

std::weak_ptr<std::vector<RePiVertex>> RePiMesh::getVertexBuffer() const
{
    return m_vertexBuffer;
}

std::weak_ptr<std::vector<uint32_t>> RePiMesh::getIndexBuffer() const
{
    return m_indexBuffer;
}

std::weak_ptr<RePiMaterial> RePiMesh::getMaterial() const
{
    return mMaterial;
}

RePiVertexTopology RePiMesh::getTopology() const
{
    return m_topology;
}

/*void RePiMesh::BindToCommandBuffer(const std::weak_ptr<RePiCommandBuffer>& CommandBuffer) const
{
    if (auto pCommandBuffer = CommandBuffer.lock())
    {
        uint32_t IndexCount = 0;
        bool HealthyDraw = true;

        if (auto pMaterial = mMaterial.lock())
        {
            pMaterial->BindToCommandBuffer(CommandBuffer);
        }

        if (auto pVertexBuffer = mVertexBuffer.lock())
        {
            pVertexBuffer->BindToCommandBuffer(CommandBuffer);
        }
        else
        {
            HealthyDraw = false;
        }

        if (auto pTopology = mTopology.lock())
        {
            pTopology->BindToCommandBuffer(CommandBuffer);
        }
        else
        {
            HealthyDraw = false;
        }

        if (auto pIndexBuffer = mIndexBuffer.lock())
        {
            pIndexBuffer->BindToCommandBuffer(CommandBuffer);
            IndexCount = pIndexBuffer->GetIndexCount();
        }
        else
        {
            HealthyDraw = false;
        }

        if (HealthyDraw && IndexCount > 0)
        {
            pCommandBuffer->AddDrawCommand(IndexCount);
        }
    }
}*/

RePi3DModel::RePi3DModel() :
    m_transform(RePiMatrix::IDENTITY)
{
}

RePiMatrix RePi3DModel::GetTransform() const
{
    return m_transform;
}

std::weak_ptr<RePiAnimator> RePi3DModel::GetAnimator() const
{
    return mAnimator;
}

uint32_t RePi3DModel::GetMeshCount() const
{
    return static_cast<uint32_t>(mMeshList.size());
}

void RePi3DModel::SetTransform(
    const RePiMatrix& Transform)
{
    m_transform = Transform;
}

void RePi3DModel::AddMesh(
    const std::weak_ptr<RePiMesh>& Mesh)
{
    mMeshList.push_back(Mesh);
}

void RePi3DModel::BindAnimator(
    const std::weak_ptr<RePiAnimator>& Animator)
{
    mAnimator = Animator;
}

/*void RePi3DModel::BindRenderResourcesToContext(const std::weak_ptr<RePiCommandBuffer>& RenderContext)
{
    if (auto pRenderContext = RenderContext.lock())
    {
        if (auto pBuffer = mBuffer.lock())
        {
            pBuffer->BindToCommandBuffer(pRenderContext);
        }

        for (auto& Mesh : mMeshList)
        {
            if (auto pMesh = Mesh.lock())
            {
                pMesh->BindToCommandBuffer(pRenderContext);
            }
        }
    }
}*/

void RePi3DModel::Update()
{
    if (auto pAnimator = mAnimator.lock())
    {
        if (pAnimator->IsPlayingAnimation())
        {
            pAnimator->TransferBoneData(m_boneTransform);
        }
    }
}
