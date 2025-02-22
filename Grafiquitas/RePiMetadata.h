
#pragma once

#include "RePiBase.h"

class RePiMetadata
{
public:
    RePiMetadata();

    ~RePiMetadata() = default;

    void SetMetadata(
        const std::string& Name = "",
        const std::string& Path = "",
        const uint32_t Key = 0u);

    std::string GetName() const;

    std::string GetPath() const;

    uint32_t GetKey() const;

private:
    std::string mName;

    std::string mPath;

    uint32_t mKey;
};
