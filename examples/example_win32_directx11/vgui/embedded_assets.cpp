#include "embedded_assets.h"

namespace
{
#include "embedded/lucide_ttf.inl"
}

namespace EmbeddedAssets
{
Blob LucideFont() { return { lucide_ttf_compressed_data, lucide_ttf_compressed_size }; }
}
