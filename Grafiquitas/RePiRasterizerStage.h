#pragma once

#include "RePiBase.h"

struct RePiVertex;
struct RePiLine;
struct RePiTriangle;
class RePiMaterial;
class RePiTexture;

struct RasteriserSettings
{
    RasteriserSettings(
        const RePiFillMode _fillMode = RePiFillMode::eWIREFRAME,
        const RePiCullMode _cullMode = RePiCullMode::eBACK,
        const bool _depthEnable = true,
        const bool _depthWrite = true,
        const RePiComparisonFunction _depthFunc = RePiComparisonFunction::eLESS,
        const RePiSampleFilter _sampleFilter = RePiSampleFilter::eFILTER_POINT,
        const bool _wireframe = true)
        : fillMode(_fillMode)
        , cullMode(_cullMode)
        , depthEnable(_depthEnable)
        , depthWrite(_depthWrite)
        , depthFunc(_depthFunc)
        , sampleFilter(_sampleFilter)
        , wireframe(_wireframe)
    {
    };

    ~RasteriserSettings() = default;

    RePiFillMode fillMode;
    RePiCullMode cullMode;
    bool depthEnable;
    bool depthWrite;
    RePiComparisonFunction depthFunc;
    RePiSampleFilter sampleFilter;
    bool wireframe;
};

struct RasterizerConstantBuffer
{
};

using PixelShader = std::function<RePiLinearColor(const RePiVertex&, const std::weak_ptr<RePiMaterial>&, const std::weak_ptr<RasterizerConstantBuffer>&)>;

class RePiRasterizerStage
{
public:
    RePiRasterizerStage() = default;
    ~RePiRasterizerStage() = default;

    void BindTriangleList(
        const std::weak_ptr<std::vector<RePiTriangle>>& TriangleList = std::weak_ptr<std::vector<RePiTriangle>>());

    void BindLineList(
        const std::weak_ptr<std::vector<RePiLine>>& LineList = std::weak_ptr<std::vector<RePiLine>>());

    void BindPointList(
        const std::weak_ptr<std::vector<RePiVertex>>& PointList = std::weak_ptr<std::vector<RePiVertex>>());

    void BindRasteriserSettings(
        const RasteriserSettings& RasteriserSettings = RasteriserSettings());

    void BindMaterial(
        const std::weak_ptr<RePiMaterial>& Material = std::weak_ptr<RePiMaterial>());

    void BindTarget(
        const std::weak_ptr<RePiTexture>& Target = std::weak_ptr<RePiTexture>());

    void BindDepth(
        const std::weak_ptr<RePiTexture>& Depth = std::weak_ptr<RePiTexture>());

    void BindConstantBuffer(
        const std::weak_ptr<RasterizerConstantBuffer>& ConstantBuffer = std::weak_ptr<RasterizerConstantBuffer>());

    void BindPixelShader(
        const PixelShader& PixelShader);

    void Execute();

private:
    RePiFloat2 ClipToUV(
        const RePiFloat4& Clip = RePiFloat4::ZERO) const;

    RePiInt2 ClipToXY(
        const RePiFloat4& Clip = RePiFloat4::ZERO) const;

    uint32_t ComputeRegionCode(
        RePiInt2& xy,
        const RePiInt2& Min = RePiInt2::ZERO,
        const RePiInt2& Max = RePiInt2::ZERO) const;

    const bool ClipLine(
        RePiInt2& xy0,
        RePiInt2& xy1,
        const RePiInt2& Min = RePiInt2::ZERO,
        const RePiInt2& Max = RePiInt2::ZERO) const;

    void DrawDepth(
        const RePiInt2& xy = RePiInt2::ZERO,
        const float Depth = 0.f) const;

    void DrawPixel(
        const RePiInt2& xy = RePiInt2::ZERO,
        const RePiLinearColor& Color = RePiLinearColor::Black) const;

    void DrawPoint(
        const RePiVertex& P) const;

    void DrawLine(
        const RePiLine& L) const;

    void DrawTriangle(
        const RePiTriangle& T) const;

    void DrawBottomTri(
        const RePiTriangle& T) const;

    void DrawTopTri(
        const RePiTriangle& T) const;

private:
    std::weak_ptr<std::vector<RePiTriangle>> mTriangleList;
    std::weak_ptr<std::vector<RePiLine>> mLineList;
    std::weak_ptr<std::vector<RePiVertex>> mPointList;
    RasteriserSettings mRasteriserSettings;
    std::weak_ptr<RePiMaterial> mMaterial;
    std::weak_ptr<RePiTexture> mTarget;
    std::weak_ptr<RePiTexture> mDepth;
    std::weak_ptr<RasterizerConstantBuffer> mConstantBuffer;
    PixelShader mPixelShader;
    RePiFloat2 mSize;

    enum REGION_CODE
    {
        INSIDE = 0,
        LEFT = 1,
        RIGTH = 2,
        BOTTOM = 4,
        TOP = 8
    };
};


/*void bitBlt(
        const Image& src,
        const Color& colorKey,
        const int32_t x = 0,
        const int32_t y = 0,
        const int32_t srcIniX = 0,
        const int32_t srcIniY = 0,
        const int32_t srcEndX = 0,
        const int32_t srcEndY = 0);

    void line(
        const Color& color,
        int32_t x0 = 0,
        int32_t y0 = 0,
        int32_t x1 = 0,
        int32_t y1 = 0);

    void bresenhamLine(
        const Color& color,
        int32_t x0 = 0,
        int32_t y0 = 0,
        int32_t x1 = 0,
        int32_t y1 = 0);

    void drawTriangle2D(const Triangle);*/

    /*int32_t computeRegionCode(
        const int32_t x = 0,
        const int32_t y = 0,
        const int32_t xMin = 0,
        const int32_t yMin = 0,
        const int32_t xMax = 0,
        const int32_t yMax = 0) const;

    const bool clipLine(
        int32_t& x0,
        int32_t& y0,
        int32_t& x1,
        int32_t& y1,
        const int32_t xMin = 0,
        const int32_t yMin = 0,
        const int32_t xMax = 0,
        const int32_t yMax = 0) const;

    void draw(Image& img,0
        int32_t x = 0,
        int32_t y = 0,
        int32_t srcIniX = 0,
        int32_t srcIniY = 0,
        int32_t srcEndX = 0,
        int32_t srcEndY = 0,
        TEXTURE_ADDRESS::E texAddressMode = TEXTURE_ADDRESS::CLAMP,
        BLEND_MODE::E blendMode = BLEND_MODE::NONE);*/


    //VertexShader m_shader;