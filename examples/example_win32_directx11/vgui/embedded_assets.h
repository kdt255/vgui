#pragma once

#include <cstddef>

namespace EmbeddedAssets
{
struct Blob
{
    const void* Data = nullptr;
    size_t      Size = 0;
};

Blob LucideFont();
}
