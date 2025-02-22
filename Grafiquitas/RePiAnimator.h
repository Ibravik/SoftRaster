#pragma once

#include "RePiMetadata.h"

class RePiMesh;

class RePiBone
{
public:
    RePiBone(
        const std::string& Name,
        const uint32_t Index = 0);

    ~RePiBone() = default;

    void SetFinalTransform(
        const RePiMatrix& FinalTransform);

    void SetTransform(
        const RePiMatrix& Transform);

    void SetOffset(
        const RePiMatrix& Offset);

    RePiMatrix GetFinalTransform() const;

    RePiMatrix GetTransform() const;

    RePiMatrix GetOffset() const;

    uint32_t GetIndex() const;

    std::string GetName() const;

public:
    std::vector<std::weak_ptr<RePiBone>> mChildren;

private:
    RePiMatrix mFinalTransform;

    RePiMatrix mTransform;

    RePiMatrix mOffset;

    uint32_t mIndex;

    std::string mName;
};

class RePiSkeleton : public RePiMetadata
{
public:
    RePiSkeleton() = default;

    ~RePiSkeleton();

    void SetRoot(
        const std::weak_ptr<RePiBone>& Bone);

    std::weak_ptr<RePiBone> GetRoot() const;

    std::weak_ptr<RePiBone> GetBone(
        const std::string& Name);

    uint32_t GetBoneIndex(
        const std::string& Name);

    uint32_t GetBoneCount() const;

private:
    std::map<std::string, std::shared_ptr<RePiBone>> mBoneMap;

    std::weak_ptr<RePiBone> mRootBone;
};

class RePiKeyFrameData
{
public:
    friend class RePiChannel;

    RePiKeyFrameData(
        const float Time = 0.f,
        const RePiFloat3& Position = RePiFloat3::ZERO,
        const RePiFloat3& Scale = RePiFloat3::ZERO,
        const RePiQuaternion& Rotation = RePiQuaternion::IDENTITY);

    ~RePiKeyFrameData() = default;

private:
    RePiFloat3 mPosition;

    RePiFloat3 mScale;

    RePiQuaternion mRotation;

    std::weak_ptr<RePiKeyFrameData> mNext;

    float mTime;
};

class RePiChannel
{
public:
    RePiChannel(
        const uint32_t BoneIndex = 0);

    ~RePiChannel();

    void AddScaleKey(
        const RePiFloat3& Scale,
        const float Time);

    void AddRotationKey(
        const RePiQuaternion& Rotation,
        const float Time);

    void AddPositionKey(
        const RePiFloat3& Position,
        const float Time);

    uint32_t GetBoneIndex() const;

    RePiMatrix GetTransformByTime(
        const float CurrentCycleTime);

private:
    void AddKeyFrameData(
        std::vector<std::shared_ptr<RePiKeyFrameData>>& KeyFrames,
        const float Time = 0.f,
        const RePiFloat3& Position = RePiFloat3::ZERO,
        const RePiFloat3& Scale = RePiFloat3::ZERO,
        const RePiQuaternion& Rotation = RePiQuaternion::IDENTITY);

    std::weak_ptr<RePiKeyFrameData> GetKeyFrameDataByTime(
        const float CurrentCicleTime,
        const std::vector<std::shared_ptr<RePiKeyFrameData>>& KeyFrames) const;

private:
    std::vector<std::shared_ptr<RePiKeyFrameData>> mScale;

    std::vector<std::shared_ptr<RePiKeyFrameData>> mRotation;

    std::vector<std::shared_ptr<RePiKeyFrameData>> mPosition;

    int32_t mBoneIndex;
};

class RePiAnimation : public RePiMetadata
{
public:
    RePiAnimation(
        const float CycleTime = 0.f);

    ~RePiAnimation();

    std::weak_ptr<RePiChannel> GetChannel(
        const std::weak_ptr<RePiBone>& Bone);

    std::weak_ptr<RePiChannel> GetChannel(
        const uint32_t BoneIndex) const;

    float GetCycleTime() const;

private:
    std::map<std::string, std::shared_ptr<RePiChannel>> mChannelMap;

    float mCycleTime;
};

class RePiAnimator : public RePiMetadata
{
public:
    RePiAnimator();

    virtual ~RePiAnimator() = default;

    void AddAnimation(
        const std::string& Name,
        const std::weak_ptr<RePiAnimation>& Animation);

    void SetMesh(
        const std::weak_ptr<RePiMesh>& Mesh);

    void SetTransform(
        const RePiMatrix& Transform);

    void SetSkeleton(
        const std::weak_ptr<RePiSkeleton>& Skeleton);

    RePiMatrix GetBoneTransform(
        const uint32_t Index) const;

    uint32_t GetAnimationCount() const;

    std::weak_ptr<RePiSkeleton> GetSkeleton() const;

    std::weak_ptr<RePiMesh> GetSkeletonMesh() const;

    void TransferBoneData(
        RePiMatrix* Dst) const;

    void PlayAnimation(
        const std::string& Name);

    void StopAnimation();

    bool IsPlayingAnimation() const;

    //virtual void BindRenderResourcesToContext(const std::weak_ptr<RePiCommandBuffer>& RenderContext) override;

    void Update(
        const float Tick);

    RePiMatrix mJoinTransform[MaxBoneCapacity];
private:
    void UpdateBoneTransform(
        const std::weak_ptr<RePiBone>& Bone,
        const RePiMatrix& Transform);

private:
    std::map<std::string, std::weak_ptr<RePiAnimation>> mAnimationMap;

    std::weak_ptr<RePiAnimation> mActiveAnimation;

    std::weak_ptr<RePiMesh> mMesh;

    std::weak_ptr<RePiSkeleton> mSkeleton;

    RePiMatrix mBoneTransform[MaxBoneCapacity];

    RePiMatrix mTransform;

    float mCurrentCycleTime;
};
