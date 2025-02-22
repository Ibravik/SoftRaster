#include "RePiResourceManager.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "RePi3DModel.h"
#include "RePiAnimator.h"
#include "RePiTexture.h"
#include "RePiMaterial.h"

RePiResourceManager::RePiResourceManager() :
    mIsReady(false)
{
}

RePiResourceManager::~RePiResourceManager()
{
    CleanupResources();
}

std::weak_ptr<RePiAnimator> RePiResourceManager::CreateAnimator(
    const std::string& FilePath)
{
    if (FilePath.empty())
    {
        return std::weak_ptr<RePiAnimator>();
    }

    std::shared_ptr<RePiAnimator> Animator = nullptr;

    auto Template = GetAnimatorTemplate(FilePath);
    if (auto pTemplate = Template.lock())
    {
        auto AnimatorName = pTemplate->GetName() + "_" + std::to_string(mAnimatorMap.size());
        auto AnimatorKey = GetHashFromString(pTemplate->GetPath() + "_" + std::to_string(mAnimatorMap.size()));

        Animator = std::make_shared<RePiAnimator>();
        Animator->SetMetadata(AnimatorName, pTemplate->GetPath(), AnimatorKey);

        Animator->SetSkeleton(pTemplate->GetSkeleton());
        Animator->SetMesh(pTemplate->GetSkeletonMesh());

        mAnimatorMap[AnimatorKey] = Animator;
    }

    return Animator;
}

std::weak_ptr<RePi3DModel> RePiResourceManager::Create3DModel(
    const std::string& FilePath)
{
    if (FilePath.empty())
    {
        return std::weak_ptr<RePi3DModel>();
    }

    std::shared_ptr<RePi3DModel> Model = nullptr;

    auto Template = Get3DModelTemplate(FilePath);
    if (auto pTemplate = Template.lock())
    {
        auto ModelName = pTemplate->GetName() + "_" + std::to_string(m3DModelMap.size());
        auto ModelKey = GetHashFromString(pTemplate->GetPath() + "_" + std::to_string(m3DModelMap.size()));

        Model = std::make_shared<RePi3DModel>();
        Model->SetMetadata(ModelName, FilePath, ModelKey);

        auto AnimatorTemplate = GetAnimatorTemplate(FilePath);
        if (auto pAnimatorTemplate = AnimatorTemplate.lock())
        {
            pAnimatorTemplate->TransferBoneData(Model->m_boneTransform);
        }

        for (uint32_t MeshIndex = 0; MeshIndex < pTemplate->GetMeshCount(); ++MeshIndex)
        {
            auto Mesh = GetMesh(GetHashFromString(FilePath + "_" + std::to_string(MeshIndex)));

            if (!Mesh.expired())
            {
                Model->AddMesh(Mesh);
            }
        }

        m3DModelMap[ModelKey] = Model;
    }

    return Model;
}

void RePiResourceManager::BindAnimations(
    const std::string& FilePath,
    const std::weak_ptr<RePiAnimator>& Animator)
{
    if (FilePath.empty())
    {
        return;
    }

    Assimp::Importer Importer;

    auto RawData = Importer.ReadFile(FilePath, aiProcess_ConvertToLeftHanded);
    if (RawData != nullptr)
    {
        if (auto pAnimator = Animator.lock())
        {
            for (uint32_t RawAnimIndex = 0; RawAnimIndex < RawData->mNumAnimations; ++RawAnimIndex)
            {
                auto& RawAnimData = RawData->mAnimations[RawAnimIndex];

                std::string AnimationName = std::string(RawAnimData->mName.C_Str());
                if (AnimationName.empty())
                {
                    AnimationName = std::to_string(RawAnimIndex);
                }
                AnimationName = FilePath + "_" + AnimationName;

                uint32_t SkeletonKey = 0u;
                if (auto pSkeleton = pAnimator->GetSkeleton().lock())
                {
                    SkeletonKey = pSkeleton->GetKey();
                }

                auto Animation = GetAnimation(GetHashFromString(AnimationName), SkeletonKey, AnimationName, FilePath, RawAnimData);
                if (!Animation.expired())
                {
                    pAnimator->AddAnimation(AnimationName, Animation);
                }
            }
        }
    }
}

void RePiResourceManager::CleanupResources()
{
    for (auto& Animator : mAnimatorMap)
    {
        Animator.second.reset();
    }
    mAnimatorMap.clear();

    for (auto& Skeleton : mSkeletonMap)
    {
        Skeleton.second.reset();
    }
    mSkeletonMap.clear();

    for (auto& Mesh : mMeshMap)
    {
        Mesh.second.reset();
    }
    mMeshMap.clear();

    for (auto& Animation : mAnimationMap)
    {
        Animation.second.reset();
    }
    mAnimationMap.clear();

    for (auto& Model : m3DModelMap)
    {
        Model.second.reset();
    }
    m3DModelMap.clear();

    for (auto& Material : mMaterialMap)
    {
        Material.second.reset();
    }
    mMaterialMap.clear();

    for (auto& Image : mImageMap)
    {
        Image.second.reset();
    }
    mImageMap.clear();
}

void RePiResourceManager::InitCustomResources()
{
    if (mIsReady)
    {
        RePiLog(RePiLogLevel::eERROR, "Custom resources already prepared");
    }
    mIsReady = true;

    mDifuseError = GetImage(GetHashFromString("data/diffuseError.bmp"), "data/diffuseError.bmp", RePiTextureUsages::eDiffuse);
}

const uint32_t RePiResourceManager::GetHashFromString(
    const std::string& String) const
{
    return static_cast<uint32_t>(Hasher(String));
}

const std::string RePiResourceManager::GetFolderPath(
    const std::string& FilePath) const
{
    if (FilePath.empty())
    {
        return std::string();
    }

    auto LastSlashIndex = FilePath.find_last_of("/\\");

    if (LastSlashIndex == std::string::npos)
    {
        return FilePath;
    }

    return FilePath.substr(0, LastSlashIndex + 1);
}

const std::string RePiResourceManager::GetFileName(
    const std::string& FilePath) const
{
    auto LastSlashIndex = FilePath.find_last_of("/\\") + 1;
    auto LastPointIndex = FilePath.find_last_of(".");

    if (LastSlashIndex == std::string::npos || LastPointIndex == std::string::npos)
    {
        return FilePath;
    }

    return FilePath.substr(LastSlashIndex, LastPointIndex - LastSlashIndex);
}

const std::string RePiResourceManager::GetFullFileName(
    const std::string& FilePath) const
{
    auto LastSlashIndex = FilePath.find_last_of("/\\");

    if (LastSlashIndex == std::string::npos)
    {
        return FilePath;
    }

    return FilePath.substr(LastSlashIndex + 1, FilePath.size());
}

const std::weak_ptr<RePiSkeleton> RePiResourceManager::GetSkeleton(
    const uint32_t Key,
    const std::string& FilePath,
    const aiScene* RawData)
{
    if (Key == 0u)
    {
        return std::weak_ptr<RePiSkeleton>();
    }

    std::shared_ptr<RePiSkeleton> Skeleton = nullptr;
    if (mSkeletonMap.find(Key) != mSkeletonMap.end())
    {
        Skeleton = mSkeletonMap[Key];
    }
    else if (nullptr != RawData)
    {
        Skeleton = std::make_shared<RePiSkeleton>();
        Skeleton->SetMetadata("", FilePath, Key);

        if (nullptr != RawData->mRootNode)
        {
            auto Bone = Skeleton->GetBone(std::string(RawData->mRootNode->mName.C_Str()));

            Skeleton->SetRoot(Bone);
        }

        ProcessRawBone(Skeleton, std::weak_ptr<RePiBone>(), RawData->mRootNode, RawData);

        mSkeletonMap[Key] = Skeleton;
    }

    return Skeleton;
}

const std::weak_ptr<RePiMesh> RePiResourceManager::GetSkeletonMesh(
    const uint32_t Key,
    const std::string& FilePath,
    const aiScene* RawData)
{
    if (Key == 0u)
    {
        return std::weak_ptr<RePiMesh>();
    }

    std::shared_ptr<RePiMesh> Mesh = nullptr;
    if (mMeshMap.find(Key) != mMeshMap.end())
    {
        Mesh = mMeshMap[Key];
    }
    else
    {
        if (auto pSkeleton = GetSkeleton(Key, FilePath, RawData).lock())
        {
            std::vector<uint32_t> IndexData;
            IndexData.reserve(static_cast<size_t>(pSkeleton->GetBoneCount() * static_cast<uint32_t>(2)));

            std::vector<RePiVertex> VertexData;
            VertexData.reserve(static_cast<size_t>(pSkeleton->GetBoneCount()));

            ProcessBone(VertexData, IndexData, pSkeleton->GetRoot());

            if (!VertexData.empty() && !IndexData.empty())
            {
                Mesh = std::make_shared<RePiMesh>();

                for (auto& Vertex : VertexData)
                {
                    Mesh->addVertex(Vertex);
                }

                for (auto& Index : IndexData)
                {
                    Mesh->addIndex(Index);
                }

                Mesh->setTopology(RePiVertexTopology::eLINELIST);

                Mesh->SetMetadata("", FilePath, Key);

                mMeshMap[Key] = Mesh;
            }
        }
    }

    return Mesh;
}

const std::weak_ptr<RePiAnimation> RePiResourceManager::GetAnimation(
    const uint32_t Key,
    const uint32_t SkeletonKey,
    const std::string& AnimationName,
    const std::string& FilePath,
    const aiAnimation* RawAnimationData)
{
    if (Key == 0u)
    {
        return std::weak_ptr<RePiAnimation>();
    }

    std::shared_ptr<RePiAnimation> Animation = nullptr;
    if (mAnimationMap.find(Key) != mAnimationMap.end())
    {
        Animation = mAnimationMap[Key];
    }
    else if (nullptr != RawAnimationData)
    {
        auto ToSeconds = 1.f / static_cast<float>(RawAnimationData->mTicksPerSecond);

        if (auto pSkeleton = GetSkeleton(SkeletonKey).lock())
        {
            Animation = std::make_shared<RePiAnimation>(ToSeconds * static_cast<float>(RawAnimationData->mDuration));
            Animation->SetMetadata(AnimationName, FilePath, Key);

            for (uint32_t RawChannelIndex = 0; RawChannelIndex < RawAnimationData->mNumChannels; ++RawChannelIndex)
            {
                auto& RawChannelData = RawAnimationData->mChannels[RawChannelIndex];

                auto BoneName = std::string(RawChannelData->mNodeName.C_Str());
                if (auto pChannel = Animation->GetChannel(pSkeleton->GetBone(BoneName)).lock())
                {
                    for (uint32_t RawKeyIndex = 0; RawKeyIndex < RawChannelData->mNumPositionKeys; ++RawKeyIndex)
                    {
                        auto& KeyValue = RawChannelData->mPositionKeys[RawKeyIndex].mValue;
                        auto& KeyTime = RawChannelData->mPositionKeys[RawKeyIndex].mTime;

                        pChannel->AddPositionKey(RePiFloat3(KeyValue.x, KeyValue.y, KeyValue.z), ToSeconds * static_cast<float>(KeyTime));
                    }

                    for (uint32_t RawKeyIndex = 0; RawKeyIndex < RawChannelData->mNumRotationKeys; ++RawKeyIndex)
                    {
                        auto& KeyValue = RawChannelData->mRotationKeys[RawKeyIndex].mValue;
                        auto& KeyTime = RawChannelData->mRotationKeys[RawKeyIndex].mTime;

                        pChannel->AddRotationKey(RePiQuaternion(KeyValue.x, KeyValue.y, KeyValue.z, KeyValue.w), ToSeconds * static_cast<float>(KeyTime));
                    }

                    for (uint32_t RawKeyIndex = 0; RawKeyIndex < RawChannelData->mNumScalingKeys; ++RawKeyIndex)
                    {
                        auto& KeyValue = RawChannelData->mScalingKeys[RawKeyIndex].mValue;
                        auto& KeyTime = RawChannelData->mScalingKeys[RawKeyIndex].mTime;

                        pChannel->AddScaleKey(RePiFloat3(KeyValue.x, KeyValue.y, KeyValue.z), ToSeconds * static_cast<float>(KeyTime));
                    }
                }
            }
        }

        mAnimationMap[Key] = Animation;
    }

    return Animation;
}

const std::weak_ptr<RePiTexture> RePiResourceManager::GetImage(
    const uint32_t Key,
    const std::string& FilePath,
    const RePiTextureUsages TextureUsage)
{
    if (Key == 0u)
    {
        return std::weak_ptr<RePiTexture>();
    }

    std::shared_ptr<RePiTexture> Image = nullptr;
    if (mImageMap.find(Key) != mImageMap.end())
    {
        Image = mImageMap[Key];
    }
    else
    {
        Image = std::make_shared<RePiTexture>();
        if(!Image->CreateFromFile(FilePath))
        {
            Image.reset();

            return std::weak_ptr<RePiTexture>();
        }

        mImageMap[Key] = Image;
    }

    return Image;
}

const std::weak_ptr<RePiMaterial> RePiResourceManager::GetMaterial(
    const uint32_t Key,
    const std::string& MaterialName,
    const std::string& FilePath,
    const aiMaterial* RawMaterialData)
{
    if (Key == 0u)
    {
        return std::weak_ptr<RePiMaterial>();
    }

    std::shared_ptr<RePiMaterial> Material = nullptr;
    if (mMaterialMap.find(Key) != mMaterialMap.end())
    {
        Material = mMaterialMap[Key];
    }
    else if(nullptr != RawMaterialData)
    {
        Material = std::make_shared<RePiMaterial>();
        Material->SetMetadata(MaterialName, FilePath, Key);

        std::string RootPath = GetFolderPath(FilePath);
        aiString Path;

        std::weak_ptr<RePiTexture> Image = mDifuseError;
        if (AI_SUCCESS == RawMaterialData->GetTexture(aiTextureType_DIFFUSE, 0, &Path))
        {
            auto TexturePath = RootPath + GetFileName(std::string(Path.C_Str())) + ".bmp";

            auto Diffuse = GetImage(GetHashFromString(TexturePath), TexturePath, RePiTextureUsages::eDiffuse);
            if (!Diffuse.expired())
            {
                Image = Diffuse;
            }
        }
        Material->AddImageResource(Image);

        mMaterialMap[Key] = Material;
    }

    return Material;
}

const std::weak_ptr<RePiMesh> RePiResourceManager::GetMesh(
    const uint32_t Key,
    const std::string& FilePath,
    const aiMesh* RawMeshData,
    const aiScene* RawData)
{
    if (Key == 0u)
    {
        return std::weak_ptr<RePiMesh>();
    }

    std::shared_ptr<RePiMesh> Mesh = nullptr;
    if (mMeshMap.find(Key) != mMeshMap.end())
    {
        Mesh = mMeshMap[Key];
    }
    else if (nullptr != RawMeshData)
    {
        std::vector<RePiVertex> VertexData;
        std::vector<uint32_t> IndexData;
        RePiVertexTopology Topology = RePiVertexTopology::eUNDEFINED;
        
        VertexData.reserve(size_t(RawMeshData->mNumVertices));
        for (uint32_t RawVertexIndex = 0; RawVertexIndex < RawMeshData->mNumVertices; ++RawVertexIndex)
        {
            RePiVertex Vertex;

            auto& RawPosition = RawMeshData->mVertices[RawVertexIndex];
            Vertex.Position = RePiFloat3(RawPosition.x, RawPosition.y, RawPosition.z);

            auto& RawUV = RawMeshData->mTextureCoords[0][RawVertexIndex];
            Vertex.TexCoord = RePiFloat2(RawUV.x, RawUV.y);

            auto& RawNormal = RawMeshData->mNormals[RawVertexIndex];
            Vertex.Normal = RePiFloat3(RawNormal.x, RawNormal.y, RawNormal.z).getSafeNormal();

            VertexData.push_back(Vertex);
        }

        IndexData.reserve(size_t(RawMeshData->mNumFaces * 3));
        for (uint32_t RawFaceIndex = 0; RawFaceIndex < RawMeshData->mNumFaces; ++RawFaceIndex)
        {
            auto& face = RawMeshData->mFaces[RawFaceIndex];

            for (UINT RawIndex = 0; RawIndex < face.mNumIndices; RawIndex++)
            {
                IndexData.push_back(face.mIndices[RawIndex]);
            }
        }

        ProcessSmothTangentBinormal(VertexData, IndexData);

        auto Skeleton = GetSkeleton(GetHashFromString(FilePath));
        if (auto pSkeleton = Skeleton.lock())
        {
            for (uint32_t RawBoneIndex = 0; RawBoneIndex < RawMeshData->mNumBones; ++RawBoneIndex)
            {
                auto& RawBone = RawMeshData->mBones[RawBoneIndex];

                for (uint32_t RawWeightIndex = 0; RawWeightIndex < RawBone->mNumWeights; ++RawWeightIndex)
                {
                    auto& RawWeights = RawBone->mWeights[RawWeightIndex];

                    auto& Vertex = VertexData[RawWeights.mVertexId];
                    uint32_t BoneIndex = pSkeleton->GetBoneIndex(std::string(RawBone->mName.C_Str()));

                    for (uint32_t WeightIndex = 0; WeightIndex < 4; ++WeightIndex)
                    {
                        if (Vertex.BoneWeight[WeightIndex] == 0.f)
                        {
                            Vertex.BoneWeight[WeightIndex] = RawWeights.mWeight;
                            Vertex.BoneIndex[WeightIndex] = BoneIndex;

                            break;
                        }
                    }
                }
            }
        }

        if (IndexData.size() % 3)
        {
            Topology = RePiVertexTopology::ePOINTLIST;
        }
        else
        {
            Topology = RePiVertexTopology::eTRIANGLELIST;
        }

        auto MaterialName = FilePath + "_" + std::to_string(RawMeshData->mMaterialIndex);
        auto Material = GetMaterial(GetHashFromString(MaterialName), MaterialName, FilePath, RawData->mMaterials[RawMeshData->mMaterialIndex]);

        Mesh = std::make_shared<RePiMesh>();

        for (auto& Vertex : VertexData)
        {
            Mesh->addVertex(Vertex);
        }

        for (auto& Index : IndexData)
        {
            Mesh->addIndex(Index);
        }

        Mesh->setTopology(Topology);

        Mesh->setMaterial(Material);

        Mesh->SetMetadata("", FilePath, Key);

        mMeshMap[Key] = Mesh;
    }

    return Mesh;
}

const std::weak_ptr<RePiAnimator> RePiResourceManager::GetAnimatorTemplate(
    const std::string& FilePath)
{
    if (FilePath.empty())
    {
        return std::weak_ptr<RePiAnimator>();
    }
    uint32_t HashKey = GetHashFromString(FilePath);

    std::shared_ptr<RePiAnimator> Template = nullptr;
    if (mAnimatorMap.find(HashKey) != mAnimatorMap.end())
    {
        Template = mAnimatorMap[HashKey];
    }
    else
    {
        Assimp::Importer Importer;

        auto RawData = Importer.ReadFile(FilePath, aiProcess_ConvertToLeftHanded);
        if (RawData != nullptr)
        {
            Template = std::make_shared<RePiAnimator>();
            Template->SetMetadata(GetFullFileName(FilePath), FilePath, HashKey);

            auto Skeleton = GetSkeleton(GetHashFromString(FilePath), FilePath, RawData);
            if (!Skeleton.expired())
            {
                Template->SetSkeleton(Skeleton);
            }

            auto Mesh = GetSkeletonMesh(GetHashFromString(FilePath), FilePath, RawData);
            if (!Mesh.expired())
            {
                Template->SetMesh(Mesh);
            }

            mAnimatorMap[HashKey] = Template;
        }
    }

    return Template;
}

const std::weak_ptr<RePi3DModel> RePiResourceManager::Get3DModelTemplate(
    const std::string& FilePath)
{
    if (FilePath.empty())
    {
        return std::weak_ptr<RePi3DModel>();
    }
    uint32_t HashKey = GetHashFromString(FilePath);

    std::shared_ptr<RePi3DModel> Template = nullptr;
    if (m3DModelMap.find(HashKey) != m3DModelMap.end())
    {
        Template = m3DModelMap[HashKey];
    }
    else
    {
        Assimp::Importer Importer;

        auto RawData = Importer.ReadFile(FilePath, aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_MaxQuality);
        if (nullptr != RawData)
        {
            Template = std::make_shared<RePi3DModel>();
            Template->SetMetadata(GetFullFileName(FilePath), FilePath, HashKey);

            auto AnimatorTemplate = GetAnimatorTemplate(FilePath);
            if (auto pAnimatorTemplate = AnimatorTemplate.lock())
            {
                pAnimatorTemplate->TransferBoneData(Template->m_boneTransform);
            }

            for (uint32_t RawMeshIndex = 0; RawMeshIndex < RawData->mNumMeshes; ++RawMeshIndex)
            {
                auto MeshKey = GetHashFromString(FilePath + "_" + std::to_string(RawMeshIndex));

                auto Mesh = GetMesh(MeshKey, FilePath, RawData->mMeshes[RawMeshIndex], RawData);
                if (!Mesh.expired())
                {
                    Template->AddMesh(Mesh);
                }
            }

            m3DModelMap[HashKey] = Template;
        }
    }

    return Template;
}

void RePiResourceManager::ProcessRawBone(
    const std::weak_ptr<RePiSkeleton>& Skeleton,
    const std::weak_ptr<RePiBone>& Parent,
    const aiNode* RawBoneData,
    const aiScene* RawData)
{
    if (nullptr == RawBoneData || nullptr == RawData)
    {
        return;
    }

    if (auto pSkeleton = Skeleton.lock())
    {
        for (uint32_t RawMeshIndex = 0; RawMeshIndex < RawBoneData->mNumMeshes; ++RawMeshIndex)
        {
            auto& RawMeshData = RawData->mMeshes[RawBoneData->mMeshes[RawMeshIndex]];

            for (uint32_t RawBoneIndex = 0; RawBoneIndex < RawMeshData->mNumBones; ++RawBoneIndex)
            {
                auto BoneData = RawMeshData->mBones[RawBoneIndex];

                auto BoneName = std::string(BoneData->mName.C_Str());
                if (auto pBone = pSkeleton->GetBone(BoneName).lock())
                {
                    auto& aiOffset = BoneData->mOffsetMatrix;
                    pBone->SetOffset(RePiMatrix(aiOffset.a1, aiOffset.b1, aiOffset.c1, aiOffset.d1,
                                                aiOffset.a2, aiOffset.b2, aiOffset.c2, aiOffset.d2,
                                                aiOffset.a3, aiOffset.b3, aiOffset.c3, aiOffset.d3,
                                                aiOffset.a4, aiOffset.b4, aiOffset.c4, aiOffset.d4));
                }
            }
        }

        auto BoneName = std::string(RawBoneData->mName.C_Str());
        if (auto pBone = pSkeleton->GetBone(BoneName).lock())
        {
            if (auto pParent = Parent.lock())
            {
                pParent->mChildren.push_back(pBone);
            }

            auto& aiTransform = RawBoneData->mTransformation;
            pBone->SetTransform(RePiMatrix(aiTransform.a1, aiTransform.b1, aiTransform.c1, aiTransform.d1,
                                           aiTransform.a2, aiTransform.b2, aiTransform.c2, aiTransform.d2,
                                           aiTransform.a3, aiTransform.b3, aiTransform.c3, aiTransform.d3,
                                           aiTransform.a4, aiTransform.b4, aiTransform.c4, aiTransform.d4));

            for (uint32_t RawChildIndex = 0; RawChildIndex < RawBoneData->mNumChildren; ++RawChildIndex)
            {
                ProcessRawBone(Skeleton, pBone, RawBoneData->mChildren[RawChildIndex], RawData);
            }
        }
    }
}

void RePiResourceManager::ProcessBone(
    std::vector<RePiVertex>& VertexData,
    std::vector<uint32_t>& IndexData,
    const std::weak_ptr<RePiBone>& Bone)
{
    if (auto pBone = Bone.lock())
    {
        RePiVertex Vertex;
        Vertex.BoneIndex[0] = static_cast<uint32_t>(VertexData.size());
        Vertex.BoneWeight.x = 1.f;
        VertexData.push_back(Vertex);

        for (auto& Child : pBone->mChildren)
        {
            if (auto pChild = Child.lock())
            {
                IndexData.push_back(pBone->GetIndex());
                IndexData.push_back(pChild->GetIndex());

                ProcessBone(VertexData, IndexData, pChild);
            }
        }
    }
}

void RePiResourceManager::ProcessSmothTangentBinormal(
    std::vector<RePiVertex>& VertexData,
    std::vector<uint32_t>& IndexData)
{
    std::vector<RePiFloat3> TangentData(VertexData.size(), RePiFloat3(0.f));
    std::vector<RePiFloat3> BinormalData(VertexData.size(), RePiFloat3(0.0f));

    for (size_t i = 0; i < IndexData.size(); i += 3)
    {
        uint32_t i0 = IndexData[i];
        uint32_t i1 = IndexData[i + 1];
        uint32_t i2 = IndexData[i + 2];

        const RePiFloat3& Pos1 = VertexData[i0].Position;
        const RePiFloat3& Pos2 = VertexData[i1].Position;
        const RePiFloat3& Pos3 = VertexData[i2].Position;

        const RePiFloat2& Tex1 = VertexData[i0].TexCoord;
        const RePiFloat2& Tex2 = VertexData[i1].TexCoord;
        const RePiFloat2& Tex3 = VertexData[i2].TexCoord;

        RePiFloat3 Edge1 = Pos2 - Pos1;
        RePiFloat3 Edge2 = Pos3 - Pos1;

        RePiFloat2 DeltaUV1 = Tex2 - Tex1;
        RePiFloat2 DeltaUV2 = Tex3 - Tex1;

        float f = 1.0f / (DeltaUV1.x * DeltaUV2.y - DeltaUV1.y * DeltaUV2.x);

        RePiFloat3 Tangent = f * (DeltaUV2.y * Edge1 - DeltaUV1.y * Edge2);
        RePiFloat3 Binormal = f * (-DeltaUV2.x * Edge1 + DeltaUV1.x * Edge2);

        TangentData[i0] += Tangent;
        TangentData[i1] += Tangent;
        TangentData[i2] += Tangent;

        BinormalData[i0] += Binormal;
        BinormalData[i1] += Binormal;
        BinormalData[i2] += Binormal;
    }

    for (size_t i = 0; i < VertexData.size(); ++i)
    {
        RePiFloat3& Normal = VertexData[i].Normal;
        RePiFloat3& Tangent = TangentData[i];
        RePiFloat3& Binormal = BinormalData[i];

        VertexData[i].Tangent = (Tangent - RePiFloat3::dot(Tangent, Normal) * Normal).getSafeNormal();
        VertexData[i].Binormal = (Binormal - RePiFloat3::dot(Binormal, Normal) * Normal).getSafeNormal();
    }
}
