#pragma once

#include "RePiBase.h"
#include "RePiModule.h"

struct RePiVertex;

class RePiBone;
class RePiAnimation;
class RePiAnimator;
class RePiMesh;
class RePi3DModel;
class RePiMaterial;
class RePiSkeleton;
class RePiTexture;

enum RePiTextureUsages
{
    eShadow = 0,
    eDiffuse,
    eNormals,
    eSpecular,
    eNotDefined = UINT32_MAX
};

class RePiResourceManager : public RePiModule<RePiResourceManager, std::function<void()>>
{
public:
    RePiResourceManager();

    virtual ~RePiResourceManager();

    void InitCustomResources();

    std::weak_ptr<RePiAnimator> CreateAnimator(
        const std::string& FilePath = "");

    std::weak_ptr<RePi3DModel> Create3DModel(
        const std::string& FilePath = "");

    void BindAnimations(
        const std::string& FilePath = "",
        const std::weak_ptr<RePiAnimator>& Animator = std::weak_ptr<RePiAnimator>());

private:
    void CleanupResources();

    const uint32_t GetHashFromString(
        const std::string& String = "") const;

    const std::string GetFolderPath(
        const std::string& FilePath = "") const;

    const std::string GetFileName(
        const std::string& FilePath = "") const;

    const std::string GetFullFileName(
        const std::string& FilePath = "") const;

    const std::weak_ptr<RePiSkeleton> GetSkeleton(
        const uint32_t Key,
        const std::string& FilePath = "",
        const aiScene* RawData = nullptr);

    const std::weak_ptr<RePiMesh> GetSkeletonMesh(
        const uint32_t Key,
        const std::string& FilePath = "",
        const aiScene* RawData = nullptr);

    const std::weak_ptr<RePiAnimation> GetAnimation(
        const uint32_t Key,
        const uint32_t SkeletonKey,
        const std::string& AnimationName = "",
        const std::string& FilePath = "",
        const aiAnimation* RawAnimationData = nullptr);

    const std::weak_ptr<RePiTexture> GetImage(
        const uint32_t Key,
        const std::string& FilePath = "",
        const RePiTextureUsages TextureUsage = RePiTextureUsages::eNotDefined);

    const std::weak_ptr<RePiMaterial> GetMaterial(
        const uint32_t Key,
        const std::string& MaterialName = "",
        const std::string& FilePath = "",
        const aiMaterial* RawMaterialData = nullptr);

    const std::weak_ptr<RePiMesh> GetMesh(
        const uint32_t Key,
        const std::string& FilePath = "",
        const aiMesh* RawMeshData = nullptr,
        const aiScene* RawData = nullptr);

    const std::weak_ptr<RePiAnimator> GetAnimatorTemplate(
        const std::string& FilePath);

    const std::weak_ptr<RePi3DModel> Get3DModelTemplate(
        const std::string& FilePath);

    void ProcessRawBone(
        const std::weak_ptr<RePiSkeleton>& Skeleton = std::weak_ptr<RePiSkeleton>(),
        const std::weak_ptr<RePiBone>& Parent = std::weak_ptr<RePiBone>(),
        const aiNode* RawBoneData = nullptr,
        const aiScene* RawData = nullptr);

    void ProcessBone(
        std::vector<RePiVertex>& VertexData,
        std::vector<uint32_t>& IndexData,
        const std::weak_ptr<RePiBone>& Bone = std::weak_ptr<RePiBone>());

    void ProcessSmothTangentBinormal(
        std::vector<RePiVertex>& VertexData,
        std::vector<uint32_t>& IndexData);

private:
    std::map<uint32_t, std::shared_ptr<RePiAnimator>> mAnimatorMap;

    std::map<uint32_t, std::shared_ptr<RePi3DModel>> m3DModelMap;

    std::map<uint32_t, std::shared_ptr<RePiSkeleton>> mSkeletonMap;

    std::map<uint32_t, std::shared_ptr<RePiMesh>> mMeshMap;

    std::map<uint32_t, std::shared_ptr<RePiAnimation>> mAnimationMap;

    std::map<uint32_t, std::shared_ptr<RePiMaterial>> mMaterialMap;

    std::map<uint32_t, std::shared_ptr<RePiTexture>> mImageMap;

    bool mIsReady;

    std::weak_ptr<RePiTexture> mDifuseError;

    std::hash<std::string> Hasher;
};
