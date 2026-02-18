set -eux

readonly TESTFILE="dive_captures/capture_2026-02-11_17-28-25/com.google.bigwheels.project_cube_xr.debug_trim_trigger_20260211T172823.gfxr"

cmake --build build/host

cmake --install build/host --prefix build/pkg

./build/host/gfxrecon-explorer/Debug/gfxrecon-explorer \
  "${TESTFILE}" \
  build/host/test.dot

dot -Tpng -O build/host/test.dot

xdg-open build/host/test.dot.png
