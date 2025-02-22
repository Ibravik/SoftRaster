#pragma once

#include "RePiBase.h"

class RePiImage
{
public:
    RePiImage();
    ~RePiImage();

    void Create(
        const RePiInt2& Size = RePiInt2::ZERO,
        const uint32_t BitsPerPixel = 0)
    {
        mWidth = Size.x;
        mHeight = Size.y;
        mBitsPerPixel = BitsPerPixel;
        mChannels = mBitsPerPixel >> 3;
        mBuffer.resize(static_cast<size_t>(GetPitch() * mHeight));
    }

    bool Encode(
        const std::string& Filename = "");

    bool Decode(
        const std::string& Filename = "");

    const int32_t GetWidth() const
    {
        return mWidth;
    }

    const int32_t GetHeight() const
    {
        return mHeight;
    }

    const uint32_t GetBytesPerPixel() const
    {
        return mBitsPerPixel >> 3;
    }

    const uint32_t GetPitch() const
    {
        return mWidth * GetBytesPerPixel();
    }

    const RePiColor GetPixel(
        const RePiInt2& xy = RePiInt2::ZERO) const;

    void SetPixel(
        const RePiColor& Color = RePiColor::Black,
        const RePiInt2& xy = RePiInt2::ZERO);

    void Clear(
        const RePiColor& Color = RePiColor::Black);

    void* GetData();

protected:
    int32_t mChannels;
    RePiInt2 mSize;
    int32_t mWidth;
    int32_t mHeight;
    int32_t mBitsPerPixel;
    std::vector<uint8_t> mBuffer;

#pragma pack(push, 1)
    struct BMPHeader
    {
        BMPHeader()
            : Signature(0u)
            , FileSize(0u)
            , OffsetData(0u)
        {
            memset(&Unused, 0, sizeof(uint32_t));
        };

        uint16_t Signature;         // File type, should be "BM" (0x4D42)
        uint32_t FileSize;          // Size of the file in bytes
        uint32_t Unused;            // Reserved, must be 0
        uint32_t OffsetData;        // Offset to the pixel data
    };

    struct DIBHeader
    {
        DIBHeader()
            : HeaderSize(0u)
            , Width(0)
            , Height(0)
            , Planes(0u)
            , BitsPerPixel(0u)
            , Compression(0u)
            , ImageSize(0u)
        {
            memset(&Unused, 0, sizeof(uint32_t) * 4);
        };

        uint32_t HeaderSize;        // Size of this header (DIB header)
        int32_t Width;              // Width of the image in pixels
        int32_t Height;             // Height of the image in pixels
        uint16_t Planes;            // Number of color planes, must be 1
        uint16_t BitsPerPixel;      // Bits per pixel (e.g., 24 for RGB, 32 for RGBA)
        uint32_t Compression;       // Compression type (0 = BI_RGB, uncompressed)
        uint32_t ImageSize;
        uint32_t Unused[4];
    };

    struct MaskHeader
    {
        MaskHeader()
            : RedMask(0u)
            , GreenMask(0u)
            , BlueMask(0u)
            , AlphaMask(0u)
        {
        };

        uint32_t RedMask;           // Bit mask for the red channel
        uint32_t GreenMask;         // Bit mask for the green channel
        uint32_t BlueMask;          // Bit mask for the blue channel
        uint32_t AlphaMask;         // Bit mask for the alpha channel
    };
#pragma pack(pop)
};
