# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(CMAKE_C_COMPILER ../buildroot/output/host/usr/bin/arm-buildroot-linux-uclibcgnueabihf-gcc)
SET(CMAKE_CXX_COMPILER ../buildroot/output/host/usr/bin/arm-buildroot-linux-uclibcgnueabihf-gcc)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  "${SYSROOT}")

#SET(SYSROOT_PATH /box3_nfsboot/buildfs/)

set(CMAKE_LIBRARY_ARCHITECTURE arm-linux-gnueabihf)

set(CMAKE_EXE_LINKER_FLAGS "--sysroot=\"${SYSROOT}\"")
set(CMAKE_MODULE_LINKER_FLAGS "--sysroot=\"${SYSROOT}\"")

message("CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
message("CMAKE_MODULE_LINKER_FLAGS: ${CMAKE_MODULE_LINKER_FLAGS}")

#set(CMAKE_EXE_LINKER_FLAGS "--rpath-link,${SYSROOT_PATH}/lib/${CMAKE_LIBRARY_ARCHITECTURE}:${SYSROOT_PATH}/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}" )
#set(CMAKE_MODULE_LINKER_FLAGS "-rpath-link,${SYSROOT_PATH}/lib/${CMAKE_LIBRARY_ARCHITECTURE}:${SYSROOT_PATH}/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}" )

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
