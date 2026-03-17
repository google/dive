#pragma once

#include <span>

#include "decode/handle_pointer_decoder.h"
#include "decode/struct_pointer_decoder.h"
#include "format/format.h"
#include "util/defines.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

template <typename T>
inline std::span<const T> MakeMetaStructSpan(StructPointerDecoder<T> decoder)
{
    return {decoder.GetMetaStructPointer(), decoder.GetLength()};
}

template <typename T>
inline std::span<const format::HandleId> MakeHandleSpan(HandlePointerDecoder<T> decoder)
{
    return {decoder.GetPointer(), decoder.GetLength()};
}

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)
