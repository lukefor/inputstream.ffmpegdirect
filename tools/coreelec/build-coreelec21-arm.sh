#!/bin/sh
# Kept for compatibility, see build-coreelec.sh
# Usage: tools/coreelec/build-coreelec21-arm.sh [build-dir]
exec "$(dirname "$0")/build-coreelec.sh" ce21-arm "$@"
