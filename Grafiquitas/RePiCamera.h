#pragma once

#include "RePiBase.h"

class RePiCamera
{
public:
    RePiCamera(
        const RePiFloat3& Position = RePiFloat3::ZERO,
        const RePiFloat3& LookAt = RePiFloat3::ZERO,
        const RePiFloat3& Up = RePiFloat3::ZERO,
        const RePiMatrix& Projection = RePiMatrix::IDENTITY,
        const float TranslationSpeed = 500.f,
        const float RotationSpeed = RePiMath::TWO_PI);

    ~RePiCamera() = default;

    RePiMatrix GetView() const;

    RePiMatrix GetProjection() const;

    RePiFloat3 GetPosition() const;

    RePiFloat3 GetUp() const;

    RePiFloat3 GetFront() const;

    RePiFloat3 GetRight() const;

    virtual void Rotate(
        const RePiFloat3& RadiansPerAxis);

    void Move(
        const RePiFloat3& UnitsPerAxis);

    //virtual void BindRenderResourcesToContext(const std::weak_ptr<RePiCommandBuffer>& RenderContext) override;

    void Update(
        const float Tick);

protected:
    virtual void UpdateView();

protected:
    RePiMatrix mInverseView;

    RePiMatrix mView;

    RePiMatrix mProjection;

    RePiFloat3 mRotation;

    RePiFloat3 mTranslation;

    const float mTranslationSpeed;

    const float mRotationSpeed;

    bool mIsDirty;
};

class RePiPerspectiveCamera : public RePiCamera
{
public:
    RePiPerspectiveCamera(
        const RePiFloat3& Position = RePiFloat3::ZERO,
        const RePiFloat3& LookAt = RePiFloat3::ZERO,
        const RePiFloat3& Up = RePiFloat3::ZERO,
        const RePiFloat2& Size = RePiFloat2::ZERO,
        const float FovAngleY = RePiMath::HALF_PI * 0.5f,
        const float NearZ = 0.f,
        const float FarZ = 10000.f,
        const float TranslationSpeed = 500.f,
        const float RotationSpeed = RePiMath::TWO_PI, 
        const bool LockToWorldAxys = true);

    virtual ~RePiPerspectiveCamera() = default;

    void Resize(
        const RePiFloat2& Size);

protected:
    virtual void UpdateView() override;

protected:
    float mFovAngleY;

    RePiFloat2 mSize;

    RePiFloat3 mUp;

    float mNearZ;

    float mFarZ;

    bool mLockToWorldAxys;
};
