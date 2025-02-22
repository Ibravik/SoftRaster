#include "RePiRasterizerStage.h"

#include "RePi3DModel.h"
#include "RePiMaterial.h"
#include "RePiTexture.h"

void RePiRasterizerStage::BindTriangleList(
    const std::weak_ptr<std::vector<RePiTriangle>>& TriangleList)
{
    mTriangleList = TriangleList;
}

void RePiRasterizerStage::BindLineList(
    const std::weak_ptr<std::vector<RePiLine>>& LineList)
{
    mLineList = LineList;
}

void RePiRasterizerStage::BindPointList(
    const std::weak_ptr<std::vector<RePiVertex>>& PointList)
{
    mPointList = PointList;
}

void RePiRasterizerStage::BindRasteriserSettings(
    const RasteriserSettings& RasteriserSettings)
{
    mRasteriserSettings = RasteriserSettings;
}

void RePiRasterizerStage::BindMaterial(
    const std::weak_ptr<RePiMaterial>& Material)
{
    mMaterial = Material;
}

void RePiRasterizerStage::BindTarget(
    const std::weak_ptr<RePiTexture>& Target)
{
    mTarget = Target;
}

void RePiRasterizerStage::BindDepth(
    const std::weak_ptr<RePiTexture>& Depth)
{
    mDepth = Depth;
}

void RePiRasterizerStage::BindConstantBuffer(
    const std::weak_ptr<RasterizerConstantBuffer>& ConstantBuffer)
{
    mConstantBuffer = ConstantBuffer;
}

void RePiRasterizerStage::BindPixelShader(
    const PixelShader& PixelShader)
{
    mPixelShader = PixelShader;
}

void RePiRasterizerStage::Execute()
{
    mSize = RePiFloat2::ZERO;

    if (auto pDepth = mDepth.lock())
    {
        mSize = pDepth->GetSize();
    }

    if (auto pTarget = mTarget.lock())
    {
        if (mSize != RePiFloat2::ZERO && mSize != pTarget->GetSize())
        {
            return;
        }

        mSize = pTarget->GetSize();
    }

    if (mPixelShader)
    {
        if (auto pTriangleList = mTriangleList.lock())
        {
#pragma omp parallel for
            for (int i = 0; i < pTriangleList->size(); ++i)
            {
                DrawTriangle(pTriangleList->at(i));
            }
        }

        if (auto pLineList = mLineList.lock())
        {
#pragma omp parallel for
            for (int i = 0; i < pLineList->size(); ++i)
            {
                DrawLine(pLineList->at(i));
            }
        }

        if (auto pPointList = mPointList.lock())
        {
#pragma omp parallel for
            for (int i = 0; i < pPointList->size(); ++i)
            {
                DrawPoint(pPointList->at(i));
            }
        }
    }
}

RePiFloat2 RePiRasterizerStage::ClipToUV(
    const RePiFloat4& Clip) const
{
    return RePiFloat2(0.5f * (Clip.x + 1.f), 0.5f * (1.f - Clip.y));
}

RePiInt2 RePiRasterizerStage::ClipToXY(
    const RePiFloat4& Clip) const
{
    RePiFloat2 Result = ClipToUV(Clip) * RePiFloat2(mSize.x - 1.f, mSize.y - 1.f);

    return RePiInt2(int32_t(Result.x), int32_t(Result.y));
}

uint32_t RePiRasterizerStage::ComputeRegionCode(
    RePiInt2& xy,
    const RePiInt2& Min,
    const RePiInt2& Max) const
{
    uint32_t code = REGION_CODE::INSIDE;

    if (xy.x < Min.x)
    {
        code |= REGION_CODE::LEFT;
    }
    else if (xy.x > Max.x)
    {
        code |= REGION_CODE::RIGTH;
    }

    if (xy.y < Min.y)
    {
        code |= REGION_CODE::BOTTOM;
    }
    else if (xy.y > Max.y)
    {
        code |= REGION_CODE::TOP;
    }

    return code;
}

const bool RePiRasterizerStage::ClipLine(
    RePiInt2& xy0,
    RePiInt2& xy1,
    const RePiInt2& Min,
    const RePiInt2& Max) const
{
    int32_t code0 = ComputeRegionCode(xy0, Min, Max);
    int32_t code1 = ComputeRegionCode(xy1, Min, Max);

    while (true)
    {
        if (!(code0 | code1))
        {
            return true;
        }
        else if (code0 & code1)
        {
            return false;
        }
        else
        {
            int32_t x0 = xy0.x, y0 = xy0.y;
            int32_t x1 = xy1.x, y1 = xy1.y;
            int32_t xMax = Max.x, yMax = Max.y;
            int32_t xMin = Min.x, yMin = Min.y;

            int32_t x = 0, y = 0;
            int32_t codeOut = code0 ? code0 : code1;
            if (codeOut & REGION_CODE::TOP)
            {
                x = x0 + (x1 - x0) * (yMax - y0) / (y1 - y0);
                y = yMax;
            }
            else if (codeOut & REGION_CODE::BOTTOM)
            {
                x = x0 + (x1 - x0) * (yMin - y0) / (y1 - y0);
                y = yMin;
            }
            else if (codeOut & REGION_CODE::LEFT)
            {
                y = y0 + (y1 - y0) * (xMin - x0) / (x1 - x0);
                x = xMin;
            }
            else if (codeOut & REGION_CODE::RIGTH)
            {
                y = y0 + (y1 - y0) * (xMax - x0) / (x1 - x0);
                x = xMax;
            }

            if (codeOut == code0)
            {
                xy0.x = x;
                xy0.y = y;

                code0 = ComputeRegionCode(xy0, Min, Max);
            }
            else
            {
                xy1.x = x;
                xy1.y = y;

                code1 = ComputeRegionCode(xy1, Min, Max);
            }
        }
    }

    return false;
}

void RePiRasterizerStage::DrawDepth(
    const RePiInt2& xy,
    const float Depth) const
{
    /*if (m_rasteriserSettings.depthEnable)
    {
        if (auto pDepth = m_depth.lock())
        {
            float depth = pDepth->sampleData(uv);

            switch (m_rasteriserSettings.depthFunc)
            {
            case LESS:
                depthPass = clip.z < depth;
                break;
            case EQUAL:
                depthPass = clip.z == depth;
                break;
            case GREATER:
                depthPass = clip.z > depth;
                break;
            case ALWAYS:
            default:
                depthPass = true;
            }

            if (depthPass)
            {
                pDepth->writeData(clip.z, int32_t(xy.x), int32_t(xy.y));
            }
        }
    }*/
}

void RePiRasterizerStage::DrawPixel(const RePiInt2& xy, const RePiLinearColor& Color) const
{
    /*RePiFloat2 uv = clipToUV(clip);
    RePiFloat2 xy = clipToXY(clip);

    // esto deberia estar en otra funcion xd para solo chekear el depth
    bool depthPass = true;
    if (m_rasteriserSettings.depthEnable)
    {
        if (auto pDepth = m_depth.lock())
        {
            float depth = pDepth->sampleData(uv);

            switch (m_rasteriserSettings.depthFunc)
            {
            case LESS:
                depthPass = clip.z < depth;
                break;
            case EQUAL:
                depthPass = clip.z == depth;
                break;
            case GREATER:
                depthPass = clip.z > depth;
                break;
            case ALWAYS:
            default:
                depthPass = true;
            }

            if (depthPass)
            {
                pDepth->writeData(clip.z, int32_t(xy.x), int32_t(xy.y));
            }
        }
    }

    if (depthPass)
    {
        if (auto pTarget = m_target.lock())
        {
            pTarget->writeColor(color, int32_t(uv.x * m_size.x), int32_t(uv.y * m_size.y));
        }
    }*/
}


void RePiRasterizerStage::DrawPoint(
    const RePiVertex& P) const
{

}

void RePiRasterizerStage::DrawLine(
    const RePiLine& L) const
{
    RePiInt2 p0 = ClipToXY(L.v0.Position);
    RePiInt2 p1 = ClipToXY(L.v1.Position);

    if (!ClipLine(p0, p1, RePiInt2::ZERO, RePiInt2(int32_t(mSize.x), int32_t(mSize.y))))
    {
        return;
    }

    int32_t x0 = int32_t(p0.x);
    int32_t y0 = int32_t(p0.y);
    int32_t x1 = int32_t(p1.x);
    int32_t y1 = int32_t(p1.y);

    int32_t dx = std::abs(x1 - x0);
    int32_t dy = std::abs(y1 - y0);

    int32_t sx = x0 < x1 ? 1 : -1;
    int32_t sy = y0 < y1 ? 1 : -1;

    int32_t err = dx - dy;
    int32_t e2 = 0;

    int32_t x = x0;
    int32_t y = y0;

   
    while (x != x1 || y != y1)
    {
        if (auto pTarget = mTarget.lock())
        {
            if (auto pBuffer = mConstantBuffer.lock())
            {
                if (mPixelShader)
                {
                    pTarget->WriteColor(RePiInt2(x, y), RePiLinearColor(RePiColor(0, 255, 0 ,255)));
                }
            }
        }
        e2 = 2 * err;

        if (e2 > -dy)
        {
            err -= dy;
            x += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y += sy;
        }
    }
}

void RePiRasterizerStage::DrawTriangle(
    const RePiTriangle& T) const
{
    bool draw = false;

    if (mRasteriserSettings.cullMode == RePiCullMode::eFRONT)
    {
        draw = T.orientation == RepiTriangleOrientation::eCW;
    }
    else if(mRasteriserSettings.cullMode == RePiCullMode::eBACK)
    {
        draw = T.orientation == RepiTriangleOrientation::eCCW;
    }

    if (!draw)
    {
        return;
    }

    if (mRasteriserSettings.wireframe)
    {
        RePiLine Line1 = RePiLine(T.v0, T.v1);
        DrawLine(Line1);

        RePiLine Line2 = RePiLine(T.v1, T.v2);
        DrawLine(Line2);

        RePiLine Line3 = RePiLine(T.v2, T.v0);
        DrawLine(Line3);
    }
    else
    {
        RePiVertex v1 = T.v0, v2 = T.v1, v3 = T.v2;
        
        RePiInt2 P = ClipToXY(v1.Position);
        v1.Position = RePiFloat4(float(P.x), float(P.y), v1.Position.z, v1.Position.w);

        P = ClipToXY(v2.Position);
        v2.Position = RePiFloat4(float(P.x), float(P.y), v2.Position.z, v2.Position.w);

        P = ClipToXY(v3.Position);
        v3.Position = RePiFloat4(float(P.x), float(P.y), v3.Position.z, v3.Position.w);

        if (v1.Position.y > v2.Position.y) std::swap(v1, v2);
        if (v1.Position.y > v3.Position.y) std::swap(v1, v3);
        if (v2.Position.y > v3.Position.y) std::swap(v2, v3);

        if (v2.Position.y == v3.Position.y)
        {
            DrawBottomTri({ v1, v2, v3 });
        }
        else if (v1.Position.y == v2.Position.y)
        {
            DrawTopTri({ v1, v2, v3 });
        }
        else
        {
            int new_x = int(v1.Position.x + (int)(0.5 + (float)(v2.Position.y - v1.Position.y) *
                (float)(v3.Position.x - v1.Position.x) /
                (float)(v3.Position.y - v1.Position.y)));

            float new_u = v1.TexCoord.x + ((v2.Position.y - v1.Position.y) *
                (v3.TexCoord.x - v1.TexCoord.x) / (v3.Position.y - v1.Position.y));

            float new_v = v1.TexCoord.y + ((v2.Position.y - v1.Position.y) *
                (v3.TexCoord.y - v1.TexCoord.y) / (v3.Position.y - v1.Position.y));

            RePiVertex new_vtx = { { float(new_x), v2.Position.y, 0.f }, { new_u, new_v } };

            DrawBottomTri({ v1, new_vtx, v2 });
            DrawTopTri({ v2, new_vtx, v3 });
        }
    }
}

void RePiRasterizerStage::DrawBottomTri(
    const RePiTriangle& T) const
{
    RePiVertex v1 = T.v0, v2 = T.v1, v3 = T.v2;
    if (v3.Position.x < v2.Position.x) std::swap(v2, v3);

    int height = int(v3.Position.y - v1.Position.y);
    if (height <= 0) return;

    float dx_left = static_cast<float>(v2.Position.x - v1.Position.x) / height;
    float dx_right = static_cast<float>(v3.Position.x - v1.Position.x) / height;

    float du_left = (v2.TexCoord.x - v1.TexCoord.x) / height;
    float dv_left = (v2.TexCoord.y - v1.TexCoord.y) / height;
    float du_right = (v3.TexCoord.x - v1.TexCoord.x) / height;
    float dv_right = (v3.TexCoord.y - v1.TexCoord.y) / height;

    float xs = v1.Position.x, xe = v1.Position.x;
    float us = v1.TexCoord.x, vs = v1.TexCoord.y;
    float ue = v1.TexCoord.x, ve = v1.TexCoord.y;

    if (mTarget.expired())
    {
        return;
    }
    auto pTarget = mTarget.lock();

    for (int y = int(v1.Position.y); y <= int(v3.Position.y); ++y)
    {
        int left = static_cast<int>(xs);
        int right = static_cast<int>(xe);
        if (left > right) std::swap(left, right);

        float u = us, v = vs;
        float du = (ue - us) / (right - left + 1);
        float dv = (ve - vs) / (right - left + 1);

        for (int x = RePiMath::max(0, left); x <= RePiMath::min(int(mSize.x) - 1, right); ++x)
        {
            RePiVertex V(RePiFloat4::ZERO, RePiFloat2(u, v));
            RePiLinearColor pixelColor = mPixelShader(V, mMaterial, mConstantBuffer);
            pTarget->WriteColor(RePiInt2(x, y), pixelColor);

            u += du;
            v += dv;
        }

        xs += dx_left;
        xe += dx_right;
        us += du_left;
        vs += dv_left;
        ue += du_right;
        ve += dv_right;
    }
}

void RePiRasterizerStage::DrawTopTri(
    const RePiTriangle& T) const
{
    RePiVertex v1 = T.v0, v2 = T.v1, v3 = T.v2;
    if (v2.Position.x < v1.Position.x) std::swap(v1, v2);

    int height = int(v3.Position.y - v1.Position.y);
    if (height <= 0) return;

    float dx_left = static_cast<float>(v3.Position.x - v1.Position.x) / height;
    float dx_right = static_cast<float>(v3.Position.x - v2.Position.x) / height;

    float du_left = (v3.TexCoord.x - v1.TexCoord.x) / height;
    float dv_left = (v3.TexCoord.y - v1.TexCoord.y) / height;
    float du_right = (v3.TexCoord.x - v2.TexCoord.x) / height;
    float dv_right = (v3.TexCoord.y - v2.TexCoord.y) / height;

    float xs = v1.Position.x, xe = v2.Position.x;
    float us = v1.TexCoord.x, vs = v1.TexCoord.y;
    float ue = v2.TexCoord.x, ve = v2.TexCoord.y;

    if (mTarget.expired())
    {
        return;
    }
    auto pTarget = mTarget.lock();

    for (int y = int(v1.Position.y); y <= int(v3.Position.y); ++y)
    {
        int left = static_cast<int>(xs);
        int right = static_cast<int>(xe);
        if (left > right) std::swap(left, right);

        float u = us, v = vs;
        float du = (ue - us) / (right - left + 1);
        float dv = (ve - vs) / (right - left + 1);

        for (int x = RePiMath::max(0, left); x <= RePiMath::min(int(mSize.x) - 1, right); ++x)
        {
            RePiVertex V(RePiFloat4::ZERO, RePiFloat2(u, v));
            RePiLinearColor pixelColor = mPixelShader(V, mMaterial, mConstantBuffer);
            pTarget->WriteColor(RePiInt2(x, y), pixelColor);

            u += du;
            v += dv;
        }

        xs += dx_left;
        xe += dx_right;
        us += du_left;
        vs += dv_left;
        ue += du_right;
        ve += dv_right;
    }
}

/*void Image::bitBlt(
    const Image& src,
    const Color& colorKey,
    const int32_t x,
    const int32_t y,
    const int32_t srcIniX,
    const int32_t srcIniY,
    const int32_t srcEndX,
    const int32_t srcEndY)
{
    if (m_pixels.empty() ||
        x >= m_width || y >= m_height ||
        srcIniX > srcEndX || srcIniY > srcEndY ||
        std::min(std::min(srcIniX, srcEndX), std::min(srcIniY, srcEndY)) < 0 ||
        std::max(srcIniX, srcEndX) >= src.getWidth() ||
        std::max(srcIniY, srcEndY) >= src.getHeight())
    {
        return;
    }

    const int32_t& srcWidth = src.getWidth() - srcIniX;
    const int32_t& srcHeight = src.getHeight() - srcIniY;
    const int32_t& endX = srcWidth + x;
    const int32_t& endY = srcHeight + y;

    if (endX < 0 || endY < 0)
    {
        return;
    }

    const int32_t& fixWidth = std::min(m_width - x, srcWidth);
    const int32_t& fixHeight = std::min(m_height - y, srcHeight);
    const auto& Bpp = getBpp();
    const auto& Pitch = getPitch();

    for (int32_t j = y, srcJ = srcIniY; j < y + fixHeight && srcJ < srcEndY; ++j, ++srcJ)
    {
        for (int32_t i = x, srcI = srcIniX; i < x + fixHeight && srcI < srcEndX; ++i, ++srcI)
        {
            const Color& pixel = src.getPixel(srcI, srcJ);
            const size_t pixelIndex = static_cast<size_t>((j * Pitch) + (i * Bpp));

            if (pixel != colorKey)
            {
                setPixel(pixel, i, j);
            }
        }
    }
}

void Image::line(
    const Color& color,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1)
{
    if (!clipLine(x0, y0, x1, y1, 0, 0, m_width, m_height))
    {
        return;
    }

    float dx = float(x1 - x0);
    float dy = float(y1 - y0);

    float steps = std::max(std::abs(dx), std::abs(dy));

    float xInc = dx / steps;
    float yInc = dy / steps;

    float x = float(x0);
    float y = float(y0);

    for (int32_t i = 0; i < static_cast<int32_t>(steps); ++i)
    {
        setPixel(color, int32_t(x), int32_t(y));

        x += xInc;
        y += yInc;
    }
}

int32_t Image::computeRegionCode(
    const int32_t x,
    const int32_t y,
    const int32_t xMin,
    const int32_t yMin,
    const int32_t xMax,
    const int32_t yMax) const
{
    int32_t code = REGION_CODE::INSIDE;

    if (x < xMin)
    {
        code |= REGION_CODE::LEFT;
    }
    else if (x > xMax)
    {
        code |= REGION_CODE::RIGTH;
    }

    if (y < yMin)
    {
        code |= REGION_CODE::BOTTOM;
    }
    else if (y > yMax)
    {
        code |= REGION_CODE::TOP;
    }

    return code;
}

const bool Image::clipLine(
    int32_t& x0,
    int32_t& y0,
    int32_t& x1,
    int32_t& y1,
    const int32_t xMin,
    const int32_t yMin,
    const int32_t xMax,
    const int32_t yMax) const
{
    int32_t code0 = computeRegionCode(x0, y0, xMin, yMin, xMax, yMax);
    int32_t code1 = computeRegionCode(x1, y1, xMin, yMin, xMax, yMax);

    while (true)
    {
        if (!(code0 | code1))
        {
            return true;
        }
        else if (code0 & code1)
        {
            return false;
        }
        else
        {
            int32_t x = 0, y = 0;
            int32_t codeOut = code0 ? code0 : code1;

            if (codeOut & REGION_CODE::TOP)
            {
                x = x0 + (x1 - x0) * (yMax - y0) / (y1 - y0);
                y = yMax;
            }
            else if (codeOut & REGION_CODE::BOTTOM)
            {
                x = x0 + (x1 - x0) * (yMin - y0) / (y1 - y0);
                y = yMin;
            }
            else if (codeOut & REGION_CODE::LEFT)
            {
                y = y0 + (y1 - y0) * (xMin - x0) / (x1 - x0);
                x = xMin;
            }
            else if (codeOut & REGION_CODE::RIGTH)
            {
                y = y0 + (y1 - y0) * (xMax - x0) / (x1 - x0);
                x = xMax;
            }

            if (codeOut == code0)
            {
                x0 = x;
                y0 = y;
                code0 = computeRegionCode(x0, y0, xMin, yMin, xMax, yMax);
            }
            else
            {
                x1 = x;
                y1 = y;
                code1 = computeRegionCode(x1, y1, xMin, yMin, xMax, yMax);
            }
        }
    }

    return false;
}

void Texture::draw(Image& img, int32_t x, int32_t y, int32_t srcIniX, int32_t srcIniY, int32_t srcEndX, int32_t srcEndY, TEXTURE_ADDRESS::E texAddressMode, BLEND_MODE::E blendMode)
{
    if (x >= img.getWidth() || y > img.getHeight()) { return; };
    if (0 == srcEndX) srcEndX = m_image.getWidth();
    if (0 == srcEndY) srcEndY = m_image.getHeight();
    if (x < 0) { srcIniX -= x; x = 0; }
    if (y < 0) { srcIniY -= y; y = 0; }

    int32_t realWidth = srcEndX - srcIniX;
    int32_t realHeight = srcEndY - srcIniY;
    if (realWidth < 0 || realHeight < 0) { return; }
    if (x + realWidth > img.getWidth()) { realWidth -= (x + realWidth) - img.getWidth(); }
    if (y + realHeight > img.getHeight()) { realHeight -= (y + realHeight) - img.getHeight(); }

    for (int32_t i = 0; i < realHeight; ++i)
    {
        for (int32_t j = 0; j < realWidth; ++j)
        {
            FloatColor srcColor = sample(static_cast<float>(j) / realWidth, static_cast<float>(i) / realHeight, texAddressMode);
            FloatColor dstColor = FloatColor(img.getPixel(x + j, y + i));

            FloatColor blendedColor;
            switch (blendMode)
            {
            case BLEND_MODE::ALPHA_BLEND:
                blendedColor = srcColor * srcColor.a + dstColor * (1.f - srcColor.a);
                break;
            case BLEND_MODE::ADDITIVE:
                blendedColor = (srcColor * srcColor.a) + dstColor;
                blendedColor.saturate();
                break;
            case BLEND_MODE::NONE:
            default:
                blendedColor = srcColor;
                break;
            }

            img.setPixel(blendedColor.toColor(), x + j, y + i);
        }
    }
}*/

