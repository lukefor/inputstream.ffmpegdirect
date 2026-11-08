#!/bin/sh
# Build inputstream.ffmpegdirect for CoreELEC 21 Amlogic-ng.arm (e.g. AVDVPlus R10) and produce an
# installable zip. Requirements (Ubuntu 24.04):
#   apt-get install g++-arm-linux-gnueabihf cmake autoconf automake autopoint libtool pkg-config nasm gettext git zip
# Usage: tools/coreelec/build-coreelec21-arm.sh [build-dir]
# Environment: KODI_SRC (existing Kodi 21 checkout), KODI_TAG (default 21.2-Omega),
#              ADDON_SRC (add-on source, default this repository), TARBALL_DIR (pre-downloaded sources)
set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ADDON_SRC=${ADDON_SRC:-$(cd "$SCRIPT_DIR/../.." && pwd)}
BUILD_ROOT=$(mkdir -p "${1:-$ADDON_SRC/build-coreelec21-arm}" && cd "${1:-$ADDON_SRC/build-coreelec21-arm}" && pwd)
KODI_TAG=${KODI_TAG:-21.2-Omega}
KODI_SRC=${KODI_SRC:-$BUILD_ROOT/xbmc}
ADDON_ID=inputstream.ffmpegdirect

if [ ! -d "$KODI_SRC/cmake/addons" ]; then
  git clone --depth 1 -b "$KODI_TAG" https://github.com/xbmc/xbmc "$KODI_SRC"
fi

# Kodi's addon build wants a definitions dir with "<id> <source dir>"
mkdir -p "$BUILD_ROOT/addons-def/$ADDON_ID"
echo "$ADDON_ID $ADDON_SRC" > "$BUILD_ROOT/addons-def/$ADDON_ID/$ADDON_ID.txt"

export COREELEC_ADDON_DEPENDS_PATH="$BUILD_ROOT/build/build/depends"

cmake -B "$BUILD_ROOT/build" \
  -DADDONS_TO_BUILD=$ADDON_ID \
  -DADDON_SRC_PREFIX="$(dirname "$ADDON_SRC")" \
  -DADDONS_DEFINITION_DIR="$BUILD_ROOT/addons-def" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$SCRIPT_DIR/coreelec21-amlogic-ng-arm.cmake" \
  -DCMAKE_INSTALL_PREFIX="$BUILD_ROOT/install" \
  -DPACKAGE_ZIP=1 \
  -DPACKAGE_DIR="$BUILD_ROOT/zips" \
  ${TARBALL_DIR:+-DTARBALL_DIR="$TARBALL_DIR"} \
  "$KODI_SRC/cmake/addons"

make -C "$BUILD_ROOT/build" -j"$(getconf _NPROCESSORS_ONLN)"

# Repackage without the versioned .so symlinks (Kodi loads the file named in addon.xml and its
# zip installer does not recreate symlinks)
STAGE="$BUILD_ROOT/stage"
rm -rf "$STAGE" && mkdir -p "$STAGE"
cp -a "$BUILD_ROOT/install/$ADDON_ID" "$STAGE/"
find "$STAGE/$ADDON_ID" -type l -delete
find "$STAGE/$ADDON_ID" -name "*.so*" -type f -exec arm-linux-gnueabihf-strip --strip-unneeded {} \;
VERSION=$(awk '/<addon/{f=1} f && /version="/{match($0,/version="[^"]*"/); print substr($0,RSTART+9,RLENGTH-10); exit}' "$STAGE/$ADDON_ID/addon.xml")
ZIP="$BUILD_ROOT/$ADDON_ID-$VERSION-CoreELEC21-Amlogic-ng.arm.zip"
rm -f "$ZIP"
(cd "$STAGE" && zip -qr "$ZIP" "$ADDON_ID")
echo "Built $ZIP"
