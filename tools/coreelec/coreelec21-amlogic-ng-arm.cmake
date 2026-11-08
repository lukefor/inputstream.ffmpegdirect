# Cross toolchain for CoreELEC 21 (Kodi 21 Omega) Amlogic-ng.arm images, e.g. AVDVPlus R10:
# 32-bit ARM hard-float userspace on Cortex-A53/A55/A73 class SoCs, glibc 2.38, GCC 13.
# Uses the Debian/Ubuntu arm-linux-gnueabihf cross compiler (Ubuntu 24.04: gcc 13, glibc 2.39;
# the resulting binary only requires GLIBC_2.38 / GLIBCXX_3.4.32).
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CORE_SYSTEM_NAME linux)
set(CORE_PLATFORM_NAME gbm)
set(CPU arm)
set(HOST arm-linux-gnueabihf)
set(ENABLE_NEON ON)

set(CMAKE_C_COMPILER ${HOST}-gcc)
set(CMAKE_CXX_COMPILER ${HOST}-g++)
set(CMAKE_AR ${HOST}-ar CACHE FILEPATH "")
set(CMAKE_RANLIB ${HOST}-ranlib CACHE FILEPATH "")
set(CMAKE_STRIP ${HOST}-strip CACHE FILEPATH "")

set(COREELEC_ARM_FLAGS "-march=armv8-a+crc -mtune=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard -fPIC")
set(CMAKE_C_FLAGS "${COREELEC_ARM_FLAGS}" CACHE STRING "")
set(CMAKE_CXX_FLAGS "${COREELEC_ARM_FLAGS}" CACHE STRING "")

# Search the cross sysroot plus the add-on's own dependency prefix (exported by the build script)
set(CMAKE_FIND_ROOT_PATH /usr/${HOST} $ENV{COREELEC_ADDON_DEPENDS_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
