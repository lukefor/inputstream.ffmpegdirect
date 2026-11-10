#!/bin/sh
# Build inputstream.ffmpegdirect for CoreELEC and produce an installable zip.
#
# Targets:
#   ce21-arm      CoreELEC 21 (Kodi 21 Omega) Amlogic-ng.arm, e.g. AVDVPlus R10
#   ce22-aarch64  CoreELEC 22 (Kodi 22 Piers) Amlogic-no.aarch64
#
# Requirements (Ubuntu 24.04):
#   apt-get install cmake autoconf automake autopoint libtool pkg-config nasm gettext git zip \
#     g++-arm-linux-gnueabihf   (ce21-arm)
#     g++-aarch64-linux-gnu     (ce22-aarch64)
#
# Usage: tools/coreelec/build-coreelec.sh <target> [build-dir]
# Environment: KODI_SRC (existing Kodi checkout of the right version), KODI_TAG (Kodi tag to clone),
#              ADDON_SRC (add-on source, default this repository), TARBALL_DIR (pre-downloaded sources)
set -e

TARGET=$1
case "$TARGET" in
  ce21-arm)
    TOOLCHAIN=coreelec21-amlogic-ng-arm.cmake
    HOST=arm-linux-gnueabihf
    DEFAULT_KODI_TAG=21.2-Omega
    ZIP_SUFFIX=CoreELEC21-Amlogic-ng.arm
    ;;
  ce22-aarch64)
    TOOLCHAIN=coreelec22-amlogic-no-aarch64.cmake
    HOST=aarch64-linux-gnu
    DEFAULT_KODI_TAG=22.0rc2-Piers
    ZIP_SUFFIX=CoreELEC22-Amlogic-no.aarch64
    ;;
  *)
    echo "Usage: $0 <ce21-arm|ce22-aarch64> [build-dir]" >&2
    exit 1
    ;;
esac

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ADDON_SRC=${ADDON_SRC:-$(cd "$SCRIPT_DIR/../.." && pwd)}
BUILD_DIR=${2:-$ADDON_SRC/build-coreelec-$TARGET}
BUILD_ROOT=$(mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR" && pwd)
KODI_TAG=${KODI_TAG:-$DEFAULT_KODI_TAG}
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
  -DCMAKE_TOOLCHAIN_FILE="$SCRIPT_DIR/$TOOLCHAIN" \
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
find "$STAGE/$ADDON_ID" -name "*.so*" -type f -exec "$HOST-strip" --strip-unneeded {} \;
VERSION=$(awk '/<addon/{f=1} f && /version="/{match($0,/version="[^"]*"/); print substr($0,RSTART+9,RLENGTH-10); exit}' "$STAGE/$ADDON_ID/addon.xml")
ZIP="$BUILD_ROOT/$ADDON_ID-$VERSION-$ZIP_SUFFIX.zip"
rm -f "$ZIP"
(cd "$STAGE" && zip -qr "$ZIP" "$ADDON_ID")
echo "Built $ZIP"
