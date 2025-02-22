#include "RePiAnimator.h"

#include "RePi3DModel.h"

RePiBone::RePiBone(
    const std::string& Name,
    const uint32_t Index)
    : mFinalTransform(RePiMatrix::IDENTITY)
    , mTransform(RePiMatrix::IDENTITY)
    , mOffset(RePiMatrix::IDENTITY)
    , mIndex(Index)
    , mName(Name)
{
}

RePiMatrix RePiBone::GetFinalTransform() const
{
    return mFinalTransform;
}

RePiMatrix RePiBone::GetTransform() const
{
    return mTransform;
}

RePiMatrix RePiBone::GetOffset() const
{
    return mOffset;
}

uint32_t RePiBone::GetIndex() const
{
    return mIndex;
}

std::string RePiBone::GetName() const
{
    return mName;
}

void RePiBone::SetFinalTransform(
    const RePiMatrix& FinalTransform)
{
    mFinalTransform = FinalTransform;
}

void RePiBone::SetTransform(
    const RePiMatrix& Transform)
{
    mTransform = Transform;
}

void RePiBone::SetOffset(
    const RePiMatrix& Offset)
{
    mOffset = Offset;
}

RePiSkeleton::~RePiSkeleton()
{
    for (auto& Bone : mBoneMap)
    {
        Bone.second.reset();
    }
    mBoneMap.clear();
}

void RePiSkeleton::SetRoot(
    const std::weak_ptr<RePiBone>& Bone)
{
    mRootBone = Bone;
}

std::weak_ptr<RePiBone> RePiSkeleton::GetRoot() const
{
    return mRootBone;
}

std::weak_ptr<RePiBone> RePiSkeleton::GetBone(
    const std::string& Name)
{
    std::shared_ptr<RePiBone> Bone = nullptr;

    if (mBoneMap.find(Name) != mBoneMap.end())
    {
        Bone = mBoneMap[Name];
    }
    else
    {
        Bone = std::make_shared<RePiBone>(Name, static_cast<int32_t>(mBoneMap.size()));

        mBoneMap[Name] = Bone;
    }

    return Bone;
}

uint32_t RePiSkeleton::GetBoneIndex(
    const std::string& Name)
{
    if (auto pBone = GetBone(Name).lock())
    {
        return pBone->GetIndex();
    }

    return 0;
}

uint32_t RePiSkeleton::GetBoneCount() const
{
    return static_cast<uint32_t>(mBoneMap.size());
}

RePiKeyFrameData::RePiKeyFrameData(
    const float Time,
    const RePiFloat3& Position,
    const RePiFloat3& Scale,
    const RePiQuaternion& Rotation)
    : mTime(Time)
    , mPosition(Position)
    , mScale(Scale)
    , mRotation(Rotation)
{
}

RePiChannel::RePiChannel(
    const uint32_t BoneIndex) :
    mBoneIndex(BoneIndex)
{
}

RePiChannel::~RePiChannel()
{
    for (auto& Key : mScale)
    {
        Key.reset();
    }
    mScale.clear();

    for (auto& Key : mRotation)
    {
        Key.reset();
    }
    mRotation.clear();

    for (auto& Key : mPosition)
    {
        Key.reset();
    }
    mPosition.clear();
}

void RePiChannel::AddScaleKey(
    const RePiFloat3& Scale,
    const float Time)
{
    AddKeyFrameData(mScale, Time, RePiFloat3::ZERO, Scale);
}

void RePiChannel::AddRotationKey(
    const RePiQuaternion& Rotation,
    const float Time)
{
    AddKeyFrameData(mRotation, Time, RePiFloat3::ZERO, RePiFloat3::ZERO, Rotation);
}

void RePiChannel::AddPositionKey(
    const RePiFloat3& Position,
    const float Time)
{
    AddKeyFrameData(mPosition, Time, Position);
}

uint32_t RePiChannel::GetBoneIndex() const
{
    return mBoneIndex;
}

RePiMatrix RePiChannel::GetTransformByTime(
    const float CurrentCycleTime)
{
    RePiMatrix Transform = RePiMatrix::IDENTITY;

    auto ScaleKey = GetKeyFrameDataByTime(CurrentCycleTime, mScale);
    if (auto pScaleKey = ScaleKey.lock())
    {
        RePiFloat3& Data = pScaleKey->mScale;

        if (auto pNextScaleKey = pScaleKey->mNext.lock())
        {
            RePiFloat3& NextData = pNextScaleKey->mScale;

            float Interpolation = (CurrentCycleTime - pScaleKey->mTime) / (pNextScaleKey->mTime - pScaleKey->mTime);
            Data = RePiMath::lerp(Data, NextData, Interpolation);
        }

        Transform = RePiScaleMatrix(Data);
    }
    
    auto RotationKey = GetKeyFrameDataByTime(CurrentCycleTime, mRotation);
    if (auto pRotationKey = RotationKey.lock())
    {
        RePiQuaternion& Data = pRotationKey->mRotation;

        if (auto pNextRotationKey = pRotationKey->mNext.lock())
        {
            RePiQuaternion& NextData = pNextRotationKey->mRotation;

            float Interpolation = (CurrentCycleTime - pRotationKey->mTime) / (pNextRotationKey->mTime - pRotationKey->mTime);
            Data = RePiQuaternion::slerp(Data, NextData, Interpolation);
        }

        Transform = Transform * RePiRotationMatrix::make(Data);
    }

    auto PositionKey = GetKeyFrameDataByTime(CurrentCycleTime, mPosition);
    if (auto pPositionKey = PositionKey.lock())
    {
        RePiFloat3& Data = pPositionKey->mPosition;

        if (auto pNextPositionKey = pPositionKey->mNext.lock())
        {
            RePiFloat3& NextData = pNextPositionKey->mPosition;

            float Interpolation = (CurrentCycleTime - pPositionKey->mTime) / (pNextPositionKey->mTime - pPositionKey->mTime);
            Data = RePiMath::lerp(Data, NextData, Interpolation);
        }

        Transform = Transform * RePiTranslationMatrix(Data);
    }

    return Transform;
}

void RePiChannel::AddKeyFrameData(
    std::vector<std::shared_ptr<RePiKeyFrameData>>& KeyFrames,
    const float Time,
    const RePiFloat3& Position,
    const RePiFloat3& Scale,
    const RePiQuaternion& Rotation)
{
    auto KeyFrame = std::make_shared<RePiKeyFrameData>(Time, Position, Scale, Rotation);

    if (!KeyFrames.empty())
    {
        KeyFrames.back()->mNext = KeyFrame;
    }
    KeyFrames.push_back(KeyFrame);
}

std::weak_ptr<RePiKeyFrameData> RePiChannel::GetKeyFrameDataByTime(
    const float CurrentCicleTime,
    const std::vector<std::shared_ptr<RePiKeyFrameData>>& KeyFrames) const
{
    if (KeyFrames.empty())
    {
        return std::weak_ptr<RePiKeyFrameData>();
    }

    std::weak_ptr<RePiKeyFrameData> KeyFrame = KeyFrames[0];

    for (auto& CurrentKeyFrame : KeyFrames)
    {
        if (CurrentCicleTime < CurrentKeyFrame->mTime)
        {
            break;
        }
        else
        {
            KeyFrame = CurrentKeyFrame;
        }
    }

    return KeyFrame;
}

RePiAnimation::RePiAnimation(
    const float CycleTime) :
    mCycleTime(CycleTime)
{
}

RePiAnimation::~RePiAnimation()
{
    for (auto& Channel : mChannelMap)
    {
        Channel.second.reset();
    }
    mChannelMap.clear();
}

std::weak_ptr<RePiChannel> RePiAnimation::GetChannel(
    const std::weak_ptr<RePiBone>& Bone)
{
    std::string BoneName;
    uint32_t BoneIndex = 0;
    if (auto pBone = Bone.lock())
    {
        BoneName = pBone->GetName();
        BoneIndex = pBone->GetIndex();

        std::shared_ptr<RePiChannel> NewChannel = nullptr;
        if (mChannelMap.find(BoneName) != mChannelMap.end())
        {
            NewChannel = mChannelMap[BoneName];
        }
        else
        {
            NewChannel = std::make_shared<RePiChannel>(BoneIndex);

            mChannelMap[BoneName] = NewChannel;
        }

        return NewChannel;
    }

    return std::weak_ptr<RePiChannel>();
}

std::weak_ptr<RePiChannel> RePiAnimation::GetChannel(
    const uint32_t BoneIndex) const
{
    std::shared_ptr<RePiChannel> NewChannel = nullptr;
    for (auto& It : mChannelMap)
    {
        NewChannel = It.second;

        if (nullptr != NewChannel && NewChannel->GetBoneIndex() == BoneIndex)
        {
            return NewChannel;
        }
    }

    return std::weak_ptr<RePiChannel>();
}

float RePiAnimation::GetCycleTime() const
{
    return mCycleTime;
}

RePiAnimator::RePiAnimator()
    : mCurrentCycleTime(0.f)
{
    memset(&mBoneTransform[0], 0, sizeof(RePiMatrix) * MaxBoneCapacity);
}

void RePiAnimator::AddAnimation(
    const std::string& Name,
    const std::weak_ptr<RePiAnimation>& Animation)
{
    if (mAnimationMap.find(Name) == mAnimationMap.end())
    {
        mAnimationMap[Name] = Animation;
    }
}

void RePiAnimator::SetMesh(
    const std::weak_ptr<RePiMesh>& Mesh)
{
    mMesh = Mesh;
}

void RePiAnimator::SetTransform(
    const RePiMatrix& Transform)
{
    mTransform = Transform;
}

void RePiAnimator::SetSkeleton(
    const std::weak_ptr<RePiSkeleton>& Skeleton)
{
    mSkeleton = Skeleton;

    if (auto pSkeleton = mSkeleton.lock())
    {
        UpdateBoneTransform(pSkeleton->GetRoot(), RePiMatrix::IDENTITY);
    }
}

RePiMatrix RePiAnimator::GetBoneTransform(
    const uint32_t Index) const
{
    uint32_t FixIndex = RePiMath::clamp(Index, static_cast<uint32_t>(0), MaxBoneCapacity);

    return mBoneTransform[FixIndex];
}

uint32_t RePiAnimator::GetAnimationCount() const
{
    return static_cast<uint32_t>(mAnimationMap.size());
}

std::weak_ptr<RePiSkeleton> RePiAnimator::GetSkeleton() const
{
    return mSkeleton;
}

std::weak_ptr<RePiMesh> RePiAnimator::GetSkeletonMesh() const
{
    return mMesh;
}

void RePiAnimator::TransferBoneData(
    RePiMatrix* Dst) const
{
    memcpy(Dst, mBoneTransform, sizeof(RePiMatrix) * MaxBoneCapacity);
}

void RePiAnimator::PlayAnimation(
    const std::string& Name)
{
    if (mAnimationMap.find(Name) != mAnimationMap.end())
    {
        mActiveAnimation = mAnimationMap[Name];
    }
    else
    {
        mActiveAnimation = std::weak_ptr<RePiAnimation>();
    }

    mCurrentCycleTime = 0.f;
}

void RePiAnimator::StopAnimation()
{
    mActiveAnimation.reset();
}

bool RePiAnimator::IsPlayingAnimation() const
{
    if (auto pActiveAnimation = mActiveAnimation.lock())
    {
        return true;
    }

    return false;
}

/*void RePiAnimator::BindRenderResourcesToContext(
    const std::weak_ptr<RePiCommandBuffer>& RenderContext)
{
    if (auto pRenderContext = RenderContext.lock())
    {
        if (auto pBuffer = mBuffer.lock())
        {
            pBuffer->BindToCommandBuffer(pRenderContext);
        }

        if (auto pMesh = mMesh.lock())
        {
            pMesh->BindToCommandBuffer(pRenderContext);
        }
    }
}*/

void RePiAnimator::Update(
    const float Tick)
{
    if (auto pActiveAnimation = mActiveAnimation.lock())
    {
        mCurrentCycleTime += Tick;

        if (mCurrentCycleTime > pActiveAnimation->GetCycleTime())
        {
            mCurrentCycleTime = 0.f;
        }

        if (auto pSkeleton = mSkeleton.lock())
        {
            UpdateBoneTransform(pSkeleton->GetRoot(), RePiMatrix::IDENTITY);
        }
    }
}

void RePiAnimator::UpdateBoneTransform(
    const std::weak_ptr<RePiBone>& Bone, const RePiMatrix& Transform)
{
    if (auto pBone = Bone.lock())
    {
        RePiMatrix FinalTransform = pBone->GetTransform();

        if (auto pActiveAnimation = mActiveAnimation.lock())
        {
            if (auto pChannel = pActiveAnimation->GetChannel(pBone->GetIndex()).lock())
            {
                FinalTransform = pChannel->GetTransformByTime(mCurrentCycleTime);
            }
        }
        FinalTransform = FinalTransform * Transform;

        for (size_t i = 0; i < pBone->mChildren.size(); ++i)
        {
            if (auto pChild = pBone->mChildren[i].lock())
            {
                UpdateBoneTransform(pChild, FinalTransform);
            }
        }

        pBone->SetFinalTransform(FinalTransform);

        mJoinTransform[pBone->GetIndex()] = pBone->GetFinalTransform();

        mBoneTransform[pBone->GetIndex()] = (pBone->GetOffset() * pBone->GetFinalTransform());
    }
}
