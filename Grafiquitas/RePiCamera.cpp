#include "RePiCamera.h"

RePiCamera::RePiCamera(
    const RePiFloat3& Position,
    const RePiFloat3& LookAt,
    const RePiFloat3& Up,
    const RePiMatrix& Projection,
    const float TranslationSpeed,
    const float RotationSpeed)
    : mRotation(RePiFloat3::ZERO)
    , mTranslation(RePiFloat3::ZERO)
    , mTranslationSpeed(TranslationSpeed)
    , mRotationSpeed(RotationSpeed)
    , mIsDirty(true)
{
    mView = RePiLookAtMatrix(Position, LookAt, Up);

    mProjection = Projection;

    mInverseView = GetView().inverseFast();
}

RePiMatrix RePiCamera::GetView() const
{
    return mView;
}

RePiMatrix RePiCamera::GetProjection() const
{
    return mProjection;
}

RePiFloat3 RePiCamera::GetPosition() const
{
    return mInverseView.getOrigin();
}

RePiFloat3 RePiCamera::GetUp() const
{
    return mInverseView.getScaledAxis(geEngineSDK::AXIS::kY).getSafeNormal();
}

RePiFloat3 RePiCamera::GetFront() const
{
    return mInverseView.getScaledAxis(geEngineSDK::AXIS::kZ).getSafeNormal();
}

RePiFloat3 RePiCamera::GetRight() const
{
    return mInverseView.getScaledAxis(geEngineSDK::AXIS::kX).getSafeNormal();
}

void RePiCamera::Update(const float Tick)
{
    if (!mIsDirty)
    {
        return;
    }

    mRotation *= mRotationSpeed * Tick;

    mTranslation *= mTranslationSpeed * Tick;

    UpdateView();
}

void RePiCamera::Rotate(const RePiFloat3& RadiansPerAxis)
{
    if (RadiansPerAxis.equals(RePiFloat3::ZERO))
    {
        return;
    }

    mRotation = RadiansPerAxis;

    mIsDirty = true;
}

void RePiCamera::Move(RePiFloat3 const& UnitsPerAxis)
{
    if (UnitsPerAxis.equals(RePiFloat3::ZERO))
    {
        return;
    }

    mTranslation = UnitsPerAxis;

    mIsDirty = true;
}

/*void RePiCamera::BindRenderResourcesToContext(const std::weak_ptr<RePiCommandBuffer>& RenderContext)
{
    if (auto pRenderContext = RenderContext.lock())
    {
        if (auto pBuffer = mBuffer.lock())
        {
            pBuffer->BindToCommandBuffer(pRenderContext);
        }
    }
}*/

void RePiCamera::UpdateView()
{
    RePiQuaternion Rotator = RePiQuaternion(GetRight(), -mRotation.x) * RePiQuaternion(GetUp(), -mRotation.y);

    RePiFloat3 Front = Rotator.rotateVector(GetFront()).getSafeNormal();

    mView = (RePiLookAtMatrix(GetPosition(), GetPosition() + Front, GetUp()) * RePiTranslationMatrix(mTranslation));

    mInverseView = GetView().inverseFast();

    mRotation = RePiFloat3::ZERO;

    mTranslation = RePiFloat3::ZERO;
}

RePiPerspectiveCamera::RePiPerspectiveCamera(
    const RePiFloat3& Position,
    const RePiFloat3& LookAt,
    const RePiFloat3& Up,
    const RePiFloat2& Size,
    const float FovAngleY,
    const float NearZ,
    const float FarZ,
    const float TranslationSpeed,
    const float RotationSpeed,
    const bool LockToWorldAxys)
    : RePiCamera(Position, LookAt, Up, RePiPerspectiveMatrix(FovAngleY, Size.x, Size.y, NearZ, FarZ), TranslationSpeed, RotationSpeed)
    , mFovAngleY(FovAngleY)
    , mSize(Size)
    , mNearZ(NearZ)
    , mFarZ(FarZ)
    , mUp(Up)
    , mLockToWorldAxys(LockToWorldAxys)
{
}

void RePiPerspectiveCamera::Resize(
    const RePiFloat2& Size)
{
    if (mSize.equals(Size) ||
        0 >= mSize.x || 0 >= mSize.y)
    {
        return;
    }
    mSize = Size;

    mIsDirty = true;
}

void RePiPerspectiveCamera::UpdateView()
{
    if (!mLockToWorldAxys)
    {
        RePiCamera::UpdateView();
    }
    else
    {
        RePiQuaternion Rotator = RePiQuaternion(GetRight(), -mRotation.x) * RePiQuaternion(GetUp(), -mRotation.y);

        RePiFloat3 Front = Rotator.rotateVector(GetFront()).getSafeNormal();

        float FdU = RePiFloat3::dot(Front, mUp);

        if (0.9f < FdU || -0.9f > FdU)
        {
            return;
        }

        mView = (RePiLookAtMatrix(GetPosition(), GetPosition() + Front, mUp) * RePiTranslationMatrix(mTranslation));

        mInverseView = GetView().inverseFast();

        mRotation = RePiFloat3::ZERO;

        mTranslation = RePiFloat3::ZERO;
    }
}
