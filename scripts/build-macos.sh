#!/usr/bin/env bash
set -euo pipefail

# Compatibility entry point. New builds should use build_mac.sh, whose default
# is a Release build plus a verified DMG. Preserve this script's old defaults.
script_directory="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export CONFIGURATION="${CONFIGURATION:-Debug}"
export PACKAGE="${PACKAGE:-0}"
exec "${script_directory}/build_mac.sh" "$@"
