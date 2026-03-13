#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

CONFIGURE_ONLY=0
if [[ "${1:-}" == "--configure-only" ]]; then
  CONFIGURE_ONLY=1
fi

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}"

if [[ "${CONFIGURE_ONLY}" -eq 0 ]]; then
  cmake --build "${BUILD_DIR}" -j"$(nproc)"
fi

echo "Done."
