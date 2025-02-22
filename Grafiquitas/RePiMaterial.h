#pragma once

#include "RePiMetadata.h"

class RePiTexture;

class RePiMaterial : public RePiMetadata
{
public:
    RePiMaterial() = default;

    ~RePiMaterial() = default;

    void AddImageResource(const std::weak_ptr<RePiTexture>& Image);

    //void BindToCommandBuffer(const std::weak_ptr<RePiCommandBuffer>& CommandBuffer);
    std::vector<std::weak_ptr<RePiTexture>> mImageList;
};
