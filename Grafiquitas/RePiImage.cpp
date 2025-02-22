#include "RePiImage.h"

static const uint16_t BMP_SIGNATURE = 0x4D42;
static const uint32_t RED_MASK_DEFAULT = 0x00FF0000;
static const uint32_t GREEN_MASK_DEFAULT = 0x0000FF00;
static const uint32_t BLUE_MASK_DEFAULT = 0x000000FF;
static const uint32_t ALPHA_MASK_DEFAULT = 0xFF000000;

RePiImage::RePiImage()
    : mWidth(0)
    , mHeight(0)
    , mBitsPerPixel(0)
    , mChannels(0)
{
}

RePiImage::~RePiImage()
{
    mBuffer.clear();
}

bool RePiImage::Encode(
    const std::string& Filename)
{
    if (mWidth <= 0 || mHeight <= 0 ||
        mBuffer.size() < static_cast<size_t>(mWidth * mHeight * GetBytesPerPixel()))
    {
        RePiLog(RePiLogLevel::eWARNING, "Invalid image data for encoding.");

        return false;
    }

    std::ofstream File(Filename, std::ios::binary);
    if (!File)
    {
        RePiLog(RePiLogLevel::eWARNING, "Unable to open file for writing: " + Filename);

        return false;
    }

    // Calculate row and data sizes
    const int32_t RowSize = (mWidth * GetBytesPerPixel() + 3) & ~3;
    const int32_t DataSize = RowSize * mHeight;

    // Write BMP header
    BMPHeader bmpHeader;
    bmpHeader.Signature = BMP_SIGNATURE;
    bmpHeader.OffsetData = sizeof(BMPHeader) + sizeof(DIBHeader);
    bmpHeader.FileSize = bmpHeader.OffsetData + DataSize;
    File.write(reinterpret_cast<const char*>(&bmpHeader), sizeof(BMPHeader));

    // Write DIB header
    DIBHeader dibHeader;
    dibHeader.HeaderSize = sizeof(DIBHeader);
    dibHeader.Width = mWidth;
    dibHeader.Height = mHeight;
    dibHeader.Planes = 1;
    dibHeader.BitsPerPixel = mBitsPerPixel;
    dibHeader.Compression = 0;
    dibHeader.ImageSize = DataSize;
    File.write(reinterpret_cast<const char*>(&dibHeader), sizeof(DIBHeader));

    // Write pixel data (bottom-up order for BMP)
    char Padding[3] = { 0 }; // Maximum padding size is 3 bytes
    const int32_t PaddingSize = RowSize - mWidth * GetBytesPerPixel();

    for (int32_t y = mHeight - 1; y >= 0; --y)
    {
        size_t RowStart = static_cast<size_t>(y * mWidth * GetBytesPerPixel());
        File.write(reinterpret_cast<const char*>(mBuffer.data() + RowStart), static_cast<size_t>(mWidth * GetBytesPerPixel()));

        if (PaddingSize > 0)
        {
            File.write(Padding, PaddingSize);
        }
    }

    return true;
}

static const int32_t countTrailingZeros(
    uint32_t value)
{
    // Create a 32-bit bitset from the integer value
    std::bitset<32> Bits(value);

    for (int32_t i = 0; i < 32; ++i)
    {
        // Return the index of the first set bit (counting from the least significant bit)
        if (Bits[i] == 1)
        {
            return i;
        }
    }

    // Return 32 if there are no set bits (all bits are zero)
    return 32;
}

bool RePiImage::Decode(
    const std::string& Filename)
{
    std::ifstream File(Filename, std::ios::binary);
    if (!File)
    {
        RePiLog(RePiLogLevel::eWARNING, "Unable to open file: " + Filename);

        return false;
    }

    // Read BMP header
    BMPHeader bmpHeader;
    File.read(reinterpret_cast<char*>(&bmpHeader), sizeof(BMPHeader));
    if (bmpHeader.Signature != BMP_SIGNATURE)
    {
        RePiLog(RePiLogLevel::eWARNING, "File " + Filename + " is not a valid BMP");

        return false;
    }

    // Read the DIB header size
    uint32_t dibHeaderSize = 0u;
    File.read(reinterpret_cast<char*>(&dibHeaderSize), sizeof(dibHeaderSize));

    // Read DIB header
    DIBHeader dibHeader;
    File.seekg(sizeof(BMPHeader), std::ios::beg);
    File.read(reinterpret_cast<char*>(&dibHeader), sizeof(DIBHeader));
    if (dibHeader.BitsPerPixel < 8 ||
        (dibHeader.Compression != 0 && dibHeader.Compression != 3))
    {
        RePiLog(RePiLogLevel::eWARNING, "Unsupported BMP format or compression in file: " + Filename);

        return false;
    }

    // Read Mask header
    MaskHeader maskHeader;
    if (dibHeader.Compression == 0)
    {
        maskHeader.RedMask = dibHeader.BitsPerPixel >= 8 ? RED_MASK_DEFAULT : 0;
        maskHeader.GreenMask = dibHeader.BitsPerPixel >= 16 ? GREEN_MASK_DEFAULT : 0;
        maskHeader.BlueMask = dibHeader.BitsPerPixel >= 24 ? BLUE_MASK_DEFAULT : 0;
        maskHeader.AlphaMask = dibHeader.BitsPerPixel >= 32 ? ALPHA_MASK_DEFAULT : 0;
    }
    else if (dibHeaderSize >= sizeof(DIBHeader) + sizeof(MaskHeader))
    {
        File.read(reinterpret_cast<char*>(&maskHeader), sizeof(MaskHeader));
    }

    // Calculate the number of channels
    mChannels += maskHeader.RedMask == 0 ? 0 : 1;
    mChannels += maskHeader.GreenMask == 0 ? 0 : 1;
    mChannels += maskHeader.BlueMask == 0 ? 0 : 1;
    mChannels += maskHeader.AlphaMask == 0 ? 0 : 1;

    // Set image properties
    mWidth = dibHeader.Width;
    mHeight = std::abs(dibHeader.Height);
    mBitsPerPixel = 8 * mChannels;
    mBuffer.resize(static_cast<size_t>(GetPitch() * mHeight));

    // Extract raw image properties
    const int32_t Bpp = dibHeader.BitsPerPixel >> 3;
    const int32_t RowSize = (mWidth * Bpp + 3) & ~3;
    const int32_t DataSize = RowSize * dibHeader.Height;
    std::vector<unsigned char> tempBuffer(DataSize);

    // Read pixel data (BMP stores rows bottom-to-top)
    File.seekg(bmpHeader.OffsetData, std::ios::beg);
    File.read(reinterpret_cast<char*>(tempBuffer.data()), DataSize);

    // Decode pixel data
    for (int32_t y = 0; y < mHeight; ++y)
    {
        int32_t srcRowIndex = (dibHeader.Height > 0 ? mHeight - 1 - y : y) * RowSize;

        for (int32_t x = 0; x < mWidth; ++x)
        {
            uint32_t pixel = 0;
            memcpy(&pixel, &tempBuffer[static_cast<size_t>(srcRowIndex + (x * Bpp))], Bpp);

            size_t pixelIndex = static_cast<size_t>(y * mWidth * GetBytesPerPixel() + x * GetBytesPerPixel());

            if (maskHeader.RedMask != 0)
            {
                mBuffer[pixelIndex++] = (pixel & maskHeader.RedMask) >> countTrailingZeros(maskHeader.RedMask);
            }

            if (maskHeader.GreenMask != 0)
            {
                mBuffer[pixelIndex++] = (pixel & maskHeader.GreenMask) >> countTrailingZeros(maskHeader.GreenMask);
            }

            if (maskHeader.BlueMask != 0)
            {
                mBuffer[pixelIndex++] = (pixel & maskHeader.BlueMask) >> countTrailingZeros(maskHeader.BlueMask);
            }

            if (maskHeader.AlphaMask != 0)
            {
                mBuffer[pixelIndex++] = (pixel & maskHeader.AlphaMask) >> countTrailingZeros(maskHeader.AlphaMask);
            }
        }
    }

    return true;
}

const RePiColor RePiImage::GetPixel(
    const RePiInt2& xy) const
{
    if (mBuffer.empty() ||
        xy.x >= mWidth || xy.x < 0 ||
        xy.y >= mHeight || xy.y < 0)
    {
        RePiLog(RePiLogLevel::eWARNING, "Unable to get pixel");

        return RePiColor::Black;
    }

    size_t PixelPos = static_cast<size_t>((xy.y * GetPitch()) + (xy.x * GetBytesPerPixel()));
    RePiColor Color = RePiColor::Black;

    Color.b = mBuffer[PixelPos++];
    Color.g = mBuffer[PixelPos++];
    Color.r = mBuffer[PixelPos++];

    if (GetBytesPerPixel() >= 4)
    {
        Color.a = mBuffer[PixelPos];
    }
    else
    {
        Color.a = 255;
    }

    return Color;
}

void RePiImage::SetPixel(
    const RePiColor& Color,
    const RePiInt2& xy)
{
    if (mBuffer.empty() ||
        xy.x >= mWidth || xy.x < 0 ||
        xy.y >= mHeight || xy.y < 0)
    {
        return;
    }

    size_t PixelPos = static_cast<size_t>((xy.y * GetPitch()) + (xy.x * GetBytesPerPixel()));

    mBuffer[PixelPos++] = Color.b;
    mBuffer[PixelPos++] = Color.g;
    mBuffer[PixelPos++] = Color.r;

    if (GetBytesPerPixel() >= 4)
    {
        mBuffer[PixelPos] = Color.a;
    }
}

void RePiImage::Clear(
    const RePiColor& Color)
{
    const auto& Bpp = GetBytesPerPixel();
    const auto& Pitch = GetPitch();

    for (size_t x = 0; x < mWidth; ++x)
    {
        memcpy(&mBuffer[x * Bpp], &Color, Bpp);
    }

    for (size_t y = 1; y < mHeight; ++y)
    {
        memcpy(&mBuffer[y * Pitch], &mBuffer[(y - 1) * Pitch], Pitch);
    }
}

void* RePiImage::GetData()
{
    return mBuffer.data();
}
