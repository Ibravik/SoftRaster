#pragma once

#include "RePiBase.h"
#include "RePiImage.h"

class RePiTexture
{
public:
    RePiTexture() = default;
    ~RePiTexture() = default;

    void Create(
        const RePiInt2& Size = RePiInt2::ZERO,
        const RePiTextureFormat Format = RePiTextureFormat::eR8G8B8A8_UNORM);

    void CreateFromImage(
        const RePiImage& Image)
    {
        mImage = Image;
    }

    bool CreateFromFile(
        const std::string& Filename = "")
    {
        return mImage.Decode(Filename);
    }

    RePiLinearColor SampleColor(
        const RePiFloat2& uv = RePiFloat2::ZERO,
        const RePiTextureAdressMode AddressMode = RePiTextureAdressMode::eCLAMP,
        const RePiSampleFilter SampleFilter = RePiSampleFilter::eFILTER_POINT);

    float SampleData(
        const RePiFloat2& uv = RePiFloat2::ZERO,
        const RePiTextureAdressMode AddressMode = RePiTextureAdressMode::eCLAMP,
        const RePiSampleFilter SampleFilter = RePiSampleFilter::eFILTER_POINT);

    void WriteColor(
        const RePiInt2 xy = RePiInt2::ZERO,
        const RePiLinearColor& Color = RePiLinearColor::Black);

    void WriteData(
        const RePiInt2 xy = RePiInt2::ZERO,
        const float data = 0.f);

    RePiFloat2 GetSize() const;

    void ClearData(
        const float ClearValue = 0.f);

    void ClearColor(
        const RePiColor& ClearColor = RePiColor::Black);

    void Save(
        const std::string& Filename = "");

    void* GetBufferData();

private:
    void AjdustTextureAddress(
        RePiFloat2& uv,
        const RePiTextureAdressMode AddressMode = RePiTextureAdressMode::eCLAMP);

    float PackFloat(
        const RePiColor& Color = RePiColor::Black) const;

    RePiColor UnpackFloat(
        const float Data = 0.f) const;

protected:
    RePiImage mImage;
};
