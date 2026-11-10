# Cross toolchain for CoreELEC 22 (Kodi 22 Piers) Amlogic-no.aarch64 images:
# 64-bit ARM userspace, glibc 2.44, GCC 16.
# Uses the Debian/Ubuntu aarch64-linux-gnu cross compiler (Ubuntu 24.04: gcc 13, glibc 2.39), which
# produces a binary needing at most GLIBC_2.39 / GLIBCXX_3.4.32, both provided by CoreELEC 22.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CORE_SYSTEM_NAME linux)
set(CORE_PLATFORM_NAME gbm)
set(CPU aarch64)
set(HOST aarch64-linux-gnu)

set(CMAKE_C_COMPILER ${HOST}-gcc)
set(CMAKE_CXX_COMPILER ${HOST}-g++)
set(CMAKE_AR ${HOST}-ar CACHE FILEPATH "")
set(CMAKE_RANLIB ${HOST}-ranlib CACHE FILEPATH "")
set(CMAKE_STRIP ${HOST}-strip CACHE FILEPATH "")

set(COREELEC_ARM_FLAGS "-march=armv8-a+crc -mtune=cortex-a55 -fPIC")
set(CMAKE_C_FLAGS "${COREELEC_ARM_FLAGS}" CACHE STRING "")
set(CMAKE_CXX_FLAGS "${COREELEC_ARM_FLAGS}" CACHE STRING "")

# Search the cross sysroot plus the add-on's own dependency prefix (exported by the build script)
set(CMAKE_FIND_ROOT_PATH /usr/${HOST} $ENV{COREELEC_ADDON_DEPENDS_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
