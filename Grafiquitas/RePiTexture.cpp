#include "RePiTexture.h"

void RePiTexture::Create(
    const RePiInt2& Size,
    const RePiTextureFormat Format)
{
    uint32_t BitsSize = 0;
    switch (Format)
    {
    case eR8G8B8A8_UNORM:
        BitsSize = 32;
        break;
    case eR32_FLOAT:
        BitsSize = 32;
        break;
    default:
        BitsSize = 0;
        break;
    }
    mImage.Create(Size, BitsSize);
}

RePiLinearColor RePiTexture::SampleColor(
    const RePiFloat2& uv,
    const RePiTextureAdressMode AddressMode,
    const RePiSampleFilter SampleFilter)
{
    return RePiLinearColor(UnpackFloat(SampleData(uv, AddressMode, SampleFilter)));
}

float RePiTexture::SampleData(
    const RePiFloat2& uv,
    const RePiTextureAdressMode AddressMode,
    const RePiSampleFilter SampleFilter)
{
    RePiFloat2 UV = uv;

    switch (SampleFilter)
    {
    case RePiSampleFilter::eFILTER_POINT:
    {
        AjdustTextureAddress(UV, AddressMode);
        int32_t x = int32_t(UV.x * (mImage.GetWidth() - 1));
        int32_t y = int32_t(UV.y * (mImage.GetHeight() - 1));

        return PackFloat(mImage.GetPixel(RePiInt2(x, y)));
    }
    case RePiSampleFilter::eFILTER_LINEAR:
    {
        AjdustTextureAddress(UV, AddressMode);
        float x = UV.x * (mImage.GetWidth() - 1);
        float y = UV.y * (mImage.GetHeight() - 1);

        int32_t x0 = int32_t(x);
        int32_t y0 = int32_t(y);
        int32_t x1 = RePiMath::min(x0 + 1, mImage.GetWidth() - 1);
        int32_t y1 = RePiMath::min(y0 + 1, mImage.GetHeight() - 1);

        float dx = x - x0;
        float dy = y - y0;

        RePiLinearColor c00(mImage.GetPixel(RePiInt2(x0, y0)));
        RePiLinearColor c10(mImage.GetPixel(RePiInt2(x1, y0)));
        RePiLinearColor c01(mImage.GetPixel(RePiInt2(x0, y1)));
        RePiLinearColor c11(mImage.GetPixel(RePiInt2(x1, y1)));

        RePiLinearColor c0 = c00 * (1.f - dx) + c10 * dx;
        RePiLinearColor c1 = c01 * (1.f - dx) + c11 * dx;

        return PackFloat((c0 * (1.f - dy) + c1 * dy).toColor(true));
    }
    default:
    {
        return 0.f;
    }
    }

    return 0.f;
}

void RePiTexture::WriteColor(
    const RePiInt2 xy,
    const RePiLinearColor& Color)
{
    mImage.SetPixel(Color.toColor(true), xy);
}

void RePiTexture::WriteData(
    const RePiInt2 xy,
    const float data)
{
    mImage.SetPixel(UnpackFloat(data), xy);
}

RePiFloat2 RePiTexture::GetSize() const
{
    return RePiFloat2(float(mImage.GetWidth()), float(mImage.GetHeight()));
}

void RePiTexture::ClearData(
    const float ClearValue)
{
    mImage.Clear(UnpackFloat(ClearValue));
}

void RePiTexture::ClearColor(
    const RePiColor& ClearColor)
{
    mImage.Clear(ClearColor);
}

void RePiTexture::Save(
    const std::string& Filename)
{
    mImage.Encode(Filename);
}

void* RePiTexture::GetBufferData()
{
    return mImage.GetData();
}

void RePiTexture::AjdustTextureAddress(
    RePiFloat2& uv,
    const RePiTextureAdressMode AddressMode)
{
    auto& u = uv.x;
    auto& v = uv.y;

    switch (AddressMode)
    {
    case RePiTextureAdressMode::eWRAP:
        u = std::fmodf(u, 1.f);
        v = std::fmodf(v, 1.f);
        break;

    case RePiTextureAdressMode::eMIRROR:
        u = std::fmodf(u, 2.f);
        v = std::fmodf(v, 2.f);

        if (u < 0.f) u = 2.f + u;
        if (v < 0.f) v = 2.f + v;
        if (u > 1.f) u = 2.f - u;
        if (v > 1.f) v = 2.f - v;
        break;

    case RePiTextureAdressMode::eMIRROR_ONCE:
        if ((u > -1.f && u < 0.f) || (u > 1.f && u < 2.f))
        {
            u = 1.f - std::fmodf(u, 1.f);
        }
        else
        {
            u = u < 0.f ? 0.f : u;
            u = u > 1.f ? 1.f : u;
        }

        if ((v > -1.f && v < 0.f) || (v > 1.f && v < 2.f))
        {
            v = 1.f - std::fmodf(v, 1.f);
        }
        else
        {
            v = v < 0.f ? 0.f : v;
            v = v > 1.f ? 1.f : v;
        }
        break;

    case RePiTextureAdressMode::eCLAMP:
    default:
        u = u < 0.f ? 0.f : u;
        u = u > 1.f ? 1.f : u;

        v = v < 0.f ? 0.f : v;
        v = v > 1.f ? 1.f : v;
        break;
    }
}

float RePiTexture::PackFloat(
    const RePiColor& Color) const
{
    uint32_t packedInt = (static_cast<uint32_t>(Color.r) << 24) |
                         (static_cast<uint32_t>(Color.g) << 16) |
                         (static_cast<uint32_t>(Color.b) << 8) |
                          static_cast<uint32_t>(Color.a);

    float result;
    std::memcpy(&result, &packedInt, sizeof(result));

    return result;
}

RePiColor RePiTexture::UnpackFloat(
    const float data) const
{
    uint32_t PackedInt = 0;
    std::memcpy(&PackedInt, &data, sizeof(PackedInt));

    RePiColor Color = RePiColor::Black;
    Color.r = (PackedInt >> 24) & 0xFF;
    Color.g = (PackedInt >> 16) & 0xFF;
    Color.b = (PackedInt >> 8) & 0xFF;
    Color.a = PackedInt & 0xFF;

    return Color;
}
