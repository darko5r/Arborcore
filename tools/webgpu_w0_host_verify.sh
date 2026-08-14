#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/browser-webgpu-w0"
mkdir -p "$OUT"
cd "$ROOT"
echo '### W0: host/browser capability inventory'
uname -a
if command -v lspci >/dev/null 2>&1; then
  lspci -nnk | grep -A4 -Ei 'VGA compatible controller|3D controller|Display controller' || true
fi
if [[ -d /dev/dri ]]; then ls -la /dev/dri; else echo '/dev/dri=ABSENT'; fi
if command -v vulkaninfo >/dev/null 2>&1; then vulkaninfo --summary 2>&1 || true; else echo 'vulkaninfo=UNAVAILABLE'; fi
firefox_version="$(firefox --version 2>/dev/null | sed -n '1p' || true)"
chrome_version="$(google-chrome-stable --version 2>/dev/null | sed -n '1p' || true)"
node_version="$(node --version 2>/dev/null || true)"
clang_version="$(clang --version 2>/dev/null | sed -n '1p' || true)"
cat > "$OUT/result.env" <<EVIDENCE
W0_HOST_RESULT=PASS
W0_KERNEL=$(uname -r)
W0_ARCH=$(uname -m)
W0_FIREFOX_VERSION=${firefox_version// /_}
W0_CHROME_VERSION=${chrome_version// /_}
W0_NODE_VERSION=${node_version// /_}
W0_CLANG_VERSION=${clang_version// /_}
W0_XDG_SESSION_TYPE=${XDG_SESSION_TYPE-UNKNOWN}
W0_DISPLAY=${DISPLAY-}
W0_WAYLAND_DISPLAY=${WAYLAND_DISPLAY-}
EVIDENCE
cat "$OUT/result.env"
echo 'PASS: W0 host inventory captured; real WebGPU availability is decided by the browser test, not inferred from Vulkan alone'
