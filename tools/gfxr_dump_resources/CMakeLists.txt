add_executable(gfxr_dump_resources main.cpp)
target_link_libraries(gfxr_dump_resources gfxrecon_decode)
# Allow includes like:
#   #include "gfxreconstruct/framework/decode/file_processor.h"
# Otherwise the includes look confusing like:
#   #include "decode/file_processor.h"
target_include_directories(gfxr_dump_resources PRIVATE "${THIRDPARTY_DIRECTORY}")