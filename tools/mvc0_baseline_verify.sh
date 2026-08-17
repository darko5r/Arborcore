#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
cd "$ROOT"
BASE_COMMIT='34214b51efefe49890b3af2b4bbee924daa27d08'
BASE_TREE='a48a7497513b6acbda0557f86daa4dac24455fda'
fail(){ printf 'FAIL: %s\n' "$*" >&2; exit 1; }
eq(){ [[ "$2" == "$3" ]] || fail "$1 expected $3 got $2"; }
[[ "$(git rev-parse HEAD)" == "$BASE_COMMIT" ]] || fail 'MVC0 HEAD moved from base during construction'
[[ "$(git rev-parse 'HEAD^{tree}')" == "$BASE_TREE" ]] || fail 'MVC0 base tree mismatch'
declare -A EXPECTED=(
["src/asm/server.asm"]="2fe79bab0896e900f9c959736125ae5b12545ad066c9d13ac031452614c60f52"
["src/asm/connection.asm"]="0bc344702b552c43d7934bc708e2aeed046b2714556d737d775943433021b646"
["src/asm/event.asm"]="fc3d9e7aef629d9746e448b272c9a2e67cd176b34ec9cfbd35096c38219cae9d"
["src/asm/http_parser.asm"]="2e612c85df2ea5e08e2b1f3a497f54f2d908e80fc250068d363013e5140ccb69"
["src/asm/http_response.asm"]="6a9d8a51f7675cadcf3ab8a578857bae64f47f3c8a7f946d388b6efccc62624e"
["src/asm/buffer.asm"]="bf6c062837f9ab274fbf71d9cbf72c40eb35658c1106fa533ceeda101c7e00ff"
["src/asm/arena.asm"]="ca2541c2dca7cb24a26d9415ffa4c6c9d5820d89895dcefde77514cd8c30bc88"
["src/asm/io.asm"]="9d135742ddc118d40678169b1e490ae944c650e327d47ddaa173fb154b21cce3"
["src/asm/route_pattern.asm"]="399dcb9a9798f8261949d769c97cce24a5a6082cd67b6a2dbfb58f94fc833bff"
["include/arborcore/assembly_abi.h"]="b54ef2d2d181fcfe980441f8711991df719d84116f03cc50c6042c7ab4eec894"
["abi/arborcore-1.symbols"]="0fb048446893dbb15a36a1d588282dcd766042f79a160e421273cbc90d9a57c6"
["abi/arborcore-1.map"]="51c6a6f8801f8d6dded4e834ed3e4e1675758a22c31bde50b0feade8644b72cd"
["abi/arborcore-1.layout"]="f710ffb0158d73c0922f889fc06de8ff9d96af425fa798d8ea814f137318137d"
["abi/arborcore-1.freeze"]="2559a4e2fa9e7e42dc9ac403f9379b8be1fdc530995e230fef0879032a5103c4"
["include/arborcore/arborcore.h"]="0281aff53690d39b99d87b8d313ad9aaa268bfbddca7f41d5f154d6d11729be5"
["src/c/runtime.c"]="373198546df4343ed7e718e893dc228fa3de460c6d880c1f078efed39ea777d5"
["src/c/request.c"]="1e32a36bd91d98f93e04726856545aa47329c51051bddf4a105b826e70c76490"
["src/c/route.c"]="65583ebbe7adf6aeba54e016cbda737394af386b4765832b0ad41c3954a96a1e"
["application/arborcore-application-ddd-mvc-foundation-1.contract"]="bb9344f5f9e90c18efcd398b5292f13e277fe289320fe8e18aaf2d61c4cc7996"
["include/arborcore/application.h"]="f0888d89eb3e6472913211e8fe8631037a175fe97e37fd173b8954caa225a274"
["src/c/application_foundation.c"]="d0baac56848d460d07aba5e2d76e1aa41d9a0829c811350e79e9028fdd1d287b"
["application/arborcore-application-capability-kernel-1.contract"]="635a7ebf1c6f5bd0e3bda9d854fc206a6dd689d220a27d8baca52bec1034ef52"
["include/arborcore/capability.h"]="fd206458962be86cc439bb9510632e218d415f14fb0641a743c51cbfbdbafe5a"
["src/c/capability_kernel.c"]="22dadae3d3e9011bd9d6b9ad2142627fc73578e54f22190300d790fb32687ba2"
["application/arborcore-application-service-runtime-1.contract"]="acd1ae3719ea5e567e50ad58f0a9e75f54a75247a615104157f83214efa409d0"
["include/arborcore/application_service.h"]="4ac545c25f72ef19b9c006c8580c53db240f6af944c809738a85516d80fa6708"
["src/c/application_service.c"]="b45623f1ad7a5f3a7ba4827a0d6e080a9dfaba58a0c06a78561ff5df9c7211b6"
["application/arborcore-application-ddd-support-1.contract"]="fcf1b06b9353125d22ef75bfb336fd14a135b8f8267c6c4df55a1e5c89d7a96d"
["include/arborcore/ddd_support.h"]="d1b97a5a5b468d7814433bcd4889bea13eee38d281e21396b8a5cf288c03bfbe"
["src/c/ddd_support.c"]="7b2aaaf073dea6c190ba279e7956bd0487df76a016478441a2bc67adf1b643c1"
["include/arborcore/renderer.h"]="b52017a125c576dc37176a76250ac7998f7c7236f857c01e5977bf71fc974847"
["src/c/renderer.c"]="4597fa57b5eb47d833fa07b02b4ecd869a3e0cbf5efb7ad934f733edb9b8ce79"
["include/arborcore/browser_surface.h"]="488e30545c65d8b44bf13a308ce3a51d4c4726ee70fa243d01ff1fecc231345e"
["src/c/browser_surface.c"]="4deb0d1ec24896b28151b70a7e7501bdfda23532725fe6a7f7ce5528dce9df02"
["browser/arborcore_host.js"]="c7fb40e47ec93796e1a68b44948b983b433ac67b152a628a02678eae297b9d4a"
)
for p in "${!EXPECTED[@]}"; do [[ -f "$p" ]] || fail "missing frozen authority $p"; eq "$p SHA-256" "$(sha256sum "$p"|awk '{print $1}')" "${EXPECTED[$p]}"; done
printf 'MVC0_BASE_COMMIT=%s\n' "$BASE_COMMIT"
printf 'MVC0_BASE_TREE=%s\n' "$BASE_TREE"
printf 'MVC0_FROZEN_AUTHORITY_PATH_COUNT=%s\n' "${#EXPECTED[@]}"
printf 'FROZEN_ASSEMBLY_SERVER_BYTE_EXACT=YES\n'
printf 'ASSEMBLY_ABI_V1_BYTE_EXACT=YES\n'
printf 'AF0_AF4_BYTE_EXACT=YES\n'
printf 'RENDERER_BROWSER_BYTE_EXACT=YES\n'
printf 'MVC0_LOWER_LAYER_RETROFIT_REQUIRED=NO\n'
printf 'PASS: MVC0 starts from exact post-AF4 authority and preserves frozen lower/presentation layers\n'
