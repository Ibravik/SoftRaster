#include "RePiMetadata.h"

RePiMetadata::RePiMetadata()
    : mName("")
    , mPath("")
    , mKey(0u)
{
}

void RePiMetadata::SetMetadata(
    const std::string& Name,
    const std::string& Path,
    const uint32_t Key)
{
    mName = Name;
    mPath = Path;
    mKey = Key;
}

std::string RePiMetadata::GetName() const
{
    return mName;
}

std::string RePiMetadata::GetPath() const
{
    return mPath;
}

uint32_t RePiMetadata::GetKey() const
{
    return mKey;
}
