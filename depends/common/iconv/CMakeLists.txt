cmake_minimum_required(VERSION 3.5)
project(iconv)

# Pass the compiler and target supplied by Kodi so cross builds don't produce host binaries
list(APPEND iconv_conf "CC=${CMAKE_C_COMPILER}"
                       "CFLAGS=${CMAKE_C_FLAGS}"
                       "LDFLAGS=${CMAKE_EXE_LINKER_FLAGS} $ENV{LDFLAGS}")
if(CMAKE_AR)
  list(APPEND iconv_conf "AR=${CMAKE_AR}")
endif()
if(CMAKE_RANLIB)
  list(APPEND iconv_conf "RANLIB=${CMAKE_RANLIB}")
endif()
if(HOST)
  list(APPEND iconv_conf --host=${HOST})
endif()

include(ExternalProject)
externalproject_add(iconv
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND ./configure
                      --prefix=${OUTPUT_DIR}
                      --without-libiconv-prefix
                      --without-libintl-prefix
                      --disable-debug
                      --disable-shared
                      --disable-dependency-tracking
                      --disable-nls
                      --enable-extra-encodings
                      --enable-static
                      --with-pic
                      ${iconv_conf}
                    BUILD_COMMAND make
                    INSTALL_COMMAND ""
                    BUILD_IN_SOURCE 1)

install(CODE "execute_process(COMMAND make install-lib WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})")
