#pragma once

#include <cstdint>
#include <filesystem>
#include <unordered_set>

#include "absl/status/statusor.h"
#include "format/format.h"
#include "util/defines.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

absl::StatusOr<std::unordered_set<uint64_t>> FindUnreferencedBlocks(
    const std::filesystem::path& capture_file,
    const std::unordered_set<format::HandleId>& unreferenced_ids);

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)
