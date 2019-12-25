cmake_minimum_required(VERSION 2.6)
project(TEST)

#add_definitions( -DBOOST_ALL_NO_LIB )
SET(TRGT "0" CACHE STRING "???")
SET(TCPREFIX "0" CACHE STRING "???")
SET(TCVERSION "0" CACHE STRING "???")
SET(SYSROOT "0" CACHE STRING "???")
SET(BUILD_ID "Fixme!!!" CACHE STRING "???")
SET(TIGERLASH_INSTALL "../opt/tigerlash")

add_definitions(-DBUILD_ID=${BUILD_ID}) #bash ${CMAKE_SOURCE_DIR}/scripts/gen-image-id.sh -e -s`)

if ("${CMAKE_BUILD_TYPE}" MATCHES "Release")
    SET(debug "")
else()
    SET(debug "debug=1")
    message("Building DEBUG configuration")
endif()

if ("${TRGT}" MATCHES "0")
    SET(target "")
    add_definitions(-DBUILD_HOST)
else()
    SET(target "target=${TRGT}")
    add_definitions(-DBUILD_TARGET)
endif()

if ("${TCPREFIX}" MATCHES "0")
    SET(tcprefix "")
else()
    SET(tcprefix "tcprefix=${TCPREFIX}")
endif()

if ("${TCVERSION}" MATCHES "0")
    SET(tcversion "")
else()
    SET(tcversion "tcversion=${TCVERSION}")
endif()

if ("${SYSROOT}" MATCHES "0")
    SET(sysroot "")
else()
    SET(sysroot "sysroot=${SYSROOT}")
endif()


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

INCLUDE(${CMAKE_SOURCE_DIR}/CMakeLists.config)
include_directories(${CMAKE_SOURCE_DIR}/include)

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -Wl,-rpath=.:./lib:./../../lib:/usr/lib:/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}")
#    SET(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -static-libstdc++ and -static-libgcc")
endif()

SET(CMAKE_SKIP_RPATH TRUE)

## Link libraries
set(Boost_USE_STATIC_LIBS ON)
set(BOOST_LIBS regex program_options thread date_time filesystem system python)
find_package(Boost COMPONENTS ${BOOST_LIBS} REQUIRED)
find_package(Threads REQUIRED)

##message("${CMAKE_MODULE_PATH}")

find_package(Jsoncpp REQUIRED)
include_directories(${Jsoncpp_INCLUDE_DIR})

#FindOpenSSL
find_package(OpenSSL REQUIRED)
FIND_PATH(OPENSSL_INCLUDE_CONF openssl/opensslconf.h)

include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_CONF})

message("OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message("OpenSSL libraries: ${OPENSSL_LIBRARIES}")
message("OpenSSL conf: ${OPENSSL_INCLUDE_CONF}")

#find_package( PythonInterp 2.7 REQUIRED )
find_package(PythonLibs 2.7 REQUIRED)
include_directories(${PYTHON_INCLUDE_DIR})
set(BUILD_SHARED_LIBS_SAVED "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)
include_directories("out/third-party/libmodbus")
include_directories("third-party/libmodbus/src")
add_subdirectory("third-party/libmodbus")
set(BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS_SAVED}")

include_directories(${Boost_INCLUDE_DIRS})
#include_directories(${SYSROOT}/usr/include/)
#find_package (PkgConfig REQUIRED)
#pkg_check_modules (GTK2 REQUIRED gtk+-2.0)

#include_directories (${GTK2_INCLUDE_DIRS})
#link_directories (${GTK2_LIBRARY_DIRS})
#add_definitions (${GTK2_CFLAGS_OTHER})

FILE(GLOB_RECURSE INC_HPP "${PROJECT_SOURCE_DIR}/include/*.hpp")
FILE(GLOB_RECURSE INC_H "${PROJECT_SOURCE_DIR}/include/*.h")
include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/third-party/jsonrpc/jsonrpc/src/main ${PROJECT_SOURCE_DIR}/third-party/jsonrpc/jsonrpc/src/main/include ${PROJECT_SOURCE_DIR}/third-party/ZBar/include ${PROJECT_SOURCE_DIR}/third-party/ZBar/zbar ${PROJECT_SOURCE_DIR}/third-party/quirc/lib ${PROJECT_SOURCE_DIR}/third-party/HighLatencyGPIO)
link_directories ("${PROJECT_SOURCE_DIR}/lib")

if(Jsoncpp_FOUND)
    message("INFO: we found LibJsoncpp")
    message(Jsoncpp_FOUND = ${Jsoncpp_FOUND})
    message(Jsoncpp_INCLUDE_DIR = ${Jsoncpp_INCLUDE_DIR})
    message(Jsoncpp_LIBRARY = ${Jsoncpp_LIBRARY})
else(Jsoncpp_FOUND)
    message("WARNING: we couldn't find libjsoncpp-dev")
endif(Jsoncpp_FOUND)

add_definitions("-DJSONRPC_API=__attribute__((visibility (\"default\")))")
set(JSONRPC "${PROJECT_SOURCE_DIR}/third-party/jsonrpc/jsonrpc/src/main/cpp/jsonreader.cpp")
set(JSONRPC ${JSONRPC} "${PROJECT_SOURCE_DIR}/third-party/jsonrpc/jsonrpc/src/main/cpp/jsonrpc.cpp")
set(JSONRPC ${JSONRPC} "${PROJECT_SOURCE_DIR}/third-party/jsonrpc/jsonrpc/src/main/cpp/jsonrpccallback.cpp")
set(JSONRPC ${JSONRPC} "${PROJECT_SOURCE_DIR}/third-party/jsonrpc/jsonrpc/src/main/cpp/jsonrpcexception.cpp")
set(JSONRPC ${JSONRPC} "${PROJECT_SOURCE_DIR}/third-party/jsonrpc/jsonrpc/src/main/cpp/jsonrpcmethod.cpp")
set(JSONRPC ${JSONRPC} "${PROJECT_SOURCE_DIR}/third-party/jsonrpc/jsonrpc/src/main/cpp/jsonrpcprivate.cpp")
set(JSONRPC ${JSONRPC} "${PROJECT_SOURCE_DIR}/third-party/jsonrpc/jsonrpc/src/main/cpp/jsonrpcstream.cpp")

set(TIGERLASH_SRCS tigerlash/main.cpp)
if(QR_ENABLED)
    add_definitions(-DQR_ENABLED)
    set(TIGERLASH_SRCS ${TIGERLASH_SRCS} tigerlash/camera/camera.cpp)

    set(ZBAR_SRC_DIR "${PROJECT_SOURCE_DIR}/third-party/ZBar/zbar")
    set(ZBAR_SRCS "${ZBAR_SRC_DIR}/img_scanner.c")
    set(ZBAR_SRCS ${ZBAR_SRCS} "${ZBAR_SRC_DIR}/image.c")
    set(ZBAR_SRCS ${ZBAR_SRCS} "${ZBAR_SRC_DIR}/symbol.c")
    set(ZBAR_SRCS ${ZBAR_SRCS} "${ZBAR_SRC_DIR}/scanner.c")
    set(ZBAR_SRCS ${ZBAR_SRCS} "${ZBAR_SRC_DIR}/error.c")
    set(ZBAR_SRCS ${ZBAR_SRCS} "${ZBAR_SRC_DIR}/refcnt.c")
    set(ZBAR_SRCS ${ZBAR_SRCS} "${ZBAR_SRC_DIR}/decoder.c")
    set(ZBAR_SRCS ${ZBAR_SRCS} "${ZBAR_SRC_DIR}/decoder/ean.c")
    set(ZBAR_SRCS ${ZBAR_SRCS} "${ZBAR_SRC_DIR}/decoder/code128.c")
    set(ZBAR_SRCS ${ZBAR_SRCS} "${ZBAR_SRC_DIR}/decoder/qr_finder.c")
    set(ZBAR_SRCS ${ZBAR_SRCS} "${ZBAR_SRC_DIR}/qrcode/qrdec.c")
    set(ZBAR_SRCS ${ZBAR_SRCS} "${ZBAR_SRC_DIR}/qrcode/util.c")
    set(ZBAR_SRCS ${ZBAR_SRCS} "${ZBAR_SRC_DIR}/qrcode/isaac.c")
    set(ZBAR_SRCS ${ZBAR_SRCS} "${ZBAR_SRC_DIR}/qrcode/bch15_5.c")
    set(ZBAR_SRCS ${ZBAR_SRCS} "${ZBAR_SRC_DIR}/qrcode/rs.c")
    set(ZBAR_SRCS ${ZBAR_SRCS} "${ZBAR_SRC_DIR}/qrcode/qrdectxt.c")
    set(ZBAR_SRCS ${ZBAR_SRCS} "${ZBAR_SRC_DIR}/qrcode/binarize.c")

    set(QUIRC_SRC_DIR "${PROJECT_SOURCE_DIR}/third-party/quirc")
    set(TIGERLASH_SRCS ${TIGERLASH_SRCS} "${QUIRC_SRC_DIR}/lib/decode.c")
    set(TIGERLASH_SRCS ${TIGERLASH_SRCS} "${QUIRC_SRC_DIR}/lib/identify.c")
    set(TIGERLASH_SRCS ${TIGERLASH_SRCS} "${QUIRC_SRC_DIR}/lib/quirc.c")
    set(TIGERLASH_SRCS ${TIGERLASH_SRCS} "${QUIRC_SRC_DIR}/lib/version_db.c")
endif()

## Target

#target_link_libraries( ${Boost_LIBRARIES})
#target_link_libraries( ${CMAKE_THREAD_LIBS_INIT})

#add_executable(12_regex_stream 12_regex_stream/main.cpp)
#target_link_libraries(12_regex_stream ${Boost_LIBRARIES} ${Jsoncpp_LIBRARY})
#target_link_libraries(12_regex_stream ${CMAKE_THREAD_LIBS_INIT})

add_executable(13_simple_python_test 13_simple_python_test/main.cpp)
target_link_libraries(13_simple_python_test ${PYTHON_LIBRARY})

add_executable(14_py 14_py/main.cpp)
target_link_libraries(14_py ${PYTHON_LIBRARY})
target_link_libraries(14_py ${Boost_LIBRARIES})
target_link_libraries (14_py ${GTK2_LIBRARIES})

add_subdirectory(third-party/pybind11)
add_executable(15_pybind 15_pybind/main.cpp)
target_link_libraries(15_pybind pybind11::embed ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${EXTRA_LIBS})

#if ("${TRGT}" MATCHES "0")
    

    #g++ main.cpp `pkg-config --libs --cflags gtk+-2.0` -I /usr/include/python2.7/ -lpython2.7 -lboost_python-py27 -o window
    #add_executable(14_py_gtk 14_py_gtk/main.cpp)
    #target_link_libraries(14_py_gtk ${PYTHON_LIBRARY})
    #target_link_libraries(14_py_gtk ${Boost_LIBRARIES})
    #target_link_libraries (14_py_gtk ${GTK2_LIBRARIES})
#endif()

add_library(jsonrpc STATIC ${JSONRPC})

set(EXTRA_LIBS jsonrpc rt z stdc++)

add_executable(16_jsonrpc_over_serial 16_jsonrpc_over_serial/main.cpp)
target_link_libraries(16_jsonrpc_over_serial ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${EXTRA_LIBS})

add_executable(17_jsonrpc_over_serial_duplex 17_jsonrpc_over_serial_duplex/main.cpp)
target_link_libraries(17_jsonrpc_over_serial_duplex ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${EXTRA_LIBS})

add_executable(18_serial_port_manager 18_serial_port_manager/main.cpp)
target_link_libraries(18_serial_port_manager ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${EXTRA_LIBS})

add_executable(19_channel_loopback 19_channel_loopback/main.cpp)
target_link_libraries(19_channel_loopback ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${EXTRA_LIBS})

add_executable(20_channels_passthrough 20_channels_passthrough/main.cpp)
target_link_libraries(20_channels_passthrough ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES}  ${EXTRA_LIBS})

add_executable(21_channels_jsonrpc 21_channels_jsonrpc/main.cpp)
target_link_libraries(21_channels_jsonrpc ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${EXTRA_LIBS})

add_executable(22_regex_stream_monitor 22_regex_stream_monitor/main.cpp)
target_link_libraries(22_regex_stream_monitor ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${EXTRA_LIBS})

add_executable(23_websocket 23_websocket/main.cpp)
target_link_libraries(23_websocket ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${EXTRA_LIBS})

add_executable(24_websocket_srv 24_websocket_srv/main.cpp)
target_link_libraries(24_websocket_srv ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${EXTRA_LIBS})

add_executable(25_websocket_jsonrpc 25_websocket_jsonrpc/main.cpp)
target_link_libraries(25_websocket_jsonrpc ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${EXTRA_LIBS})

add_executable(26_server_mgr 26_server_mgr/main.cpp)
target_link_libraries(26_server_mgr ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${EXTRA_LIBS})

# TODO: Need update tree of code
add_executable(28_serial_test tests/serial/main.cpp)
target_link_libraries(28_serial_test ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${EXTRA_LIBS})

add_executable(29_session_mgr 29_session_mgr/main.cpp)
target_link_libraries(29_session_mgr ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${EXTRA_LIBS})

add_executable(tigerlash ${JSONRPC} ${TIGERLASH_SRCS} ${ZBAR_SRCS} ${INC_HPP} ${INC_H})
target_link_libraries(tigerlash ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${EXTRA_LIBS})


set(ORANGE_SRCS orange_controller/main.cpp)
set(ORANGE_SRCS ${ORANGE_SRCS} orange_controller/modbus.cpp)
set(GPIO_HL_DIR "${PROJECT_SOURCE_DIR}/third-party/HighLatencyGPIO")
set(ORANGE_SRCS ${ORANGE_SRCS} "${GPIO_HL_DIR}/GPIO.cc")

add_executable(orange_quest ${JSONRPC} ${ORANGE_SRCS} ${INC_HPP} ${INC_H} )
target_link_libraries(orange_quest ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${EXTRA_LIBS} pybind11::embed modbus)

install ( TARGETS tigerlash
    DESTINATION ${TIGERLASH_INSTALL}
    PERMISSIONS OWNER_READ OWNER_EXECUTE
)

execute_process(COMMAND readlink -f -n tigerlash/keylogger.hex
    OUTPUT_VARIABLE KEYLOGGER_PATH
    RESULT_VARIABLE KEYLOGGER_RESULT
)

install(FILES scripts/rootfs-updater-stage0.sh scripts/rootfs-updater-stage1.sh
    DESTINATION ${TIGERLASH_INSTALL}
    PERMISSIONS OWNER_READ OWNER_EXECUTE
)

install(FILES configs/tigerlash.json ${KEYLOGGER_PATH}
    DESTINATION ${TIGERLASH_INSTALL}
)
