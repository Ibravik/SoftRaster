#include "RePiMaterial.h"

#include "RePiTexture.h"

void RePiMaterial::AddImageResource(
    const std::weak_ptr<RePiTexture>& Image)
{
    mImageList.push_back(Image);
}

/*void RePiMaterial::BindToCommandBuffer(const std::weak_ptr<RePiCommandBuffer>& CommandBuffer)
{
    if (auto pCommandBuffer = CommandBuffer.lock())
    {
        for (auto& Image : mImageList)
        {
            if (auto pImage = Image.lock())
            {
                pImage->BindToCommandBuffer(CommandBuffer);
            }
        }
    }
}*/
