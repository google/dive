#pragma once

#include <cstdint>
#include <unordered_set>

#include "format/format.h"
#include "util/defines.h"

// Don't include in a header file!

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

using HandleId = ::gfxrecon::format::HandleId;
using HandleSet = std::unordered_set<HandleId>;
using BlockIndexSet = std::unordered_set<uint64_t>;

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)
