cmake_minimum_required(VERSION 2.6)
project(Questomatic)

#add_definitions( -DBOOST_ALL_NO_LIB )
SET(TRGT "0" CACHE STRING "???")
SET(TCPREFIX "" CACHE STRING "???")
SET(TCVERSION "" CACHE STRING "???")
SET(SYSROOT "" CACHE STRING "???")
SET(BUILD_ID "Fixme!!!" CACHE STRING "???")

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

FILE(GLOB_RECURSE INC_HPP "${PROJECT_SOURCE_DIR}/include/*.hpp")
FILE(GLOB_RECURSE INC_H "${PROJECT_SOURCE_DIR}/include/*.h")
include_directories(${PROJECT_SOURCE_DIR}/include
${PROJECT_SOURCE_DIR}/third-party/jsonrpc/src/main
${PROJECT_SOURCE_DIR}/third-party/jsonrpc/src/main/include
${PROJECT_SOURCE_DIR}/third-party/HighLatencyGPIO)
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
set(JSONRPC "${PROJECT_SOURCE_DIR}/third-party/jsonrpc/src/main/cpp/jsonreader.cpp")
set(JSONRPC ${JSONRPC} "${PROJECT_SOURCE_DIR}/third-party/jsonrpc/src/main/cpp/jsonrpc.cpp")
set(JSONRPC ${JSONRPC} "${PROJECT_SOURCE_DIR}/third-party/jsonrpc/src/main/cpp/jsonrpccallback.cpp")
set(JSONRPC ${JSONRPC} "${PROJECT_SOURCE_DIR}/third-party/jsonrpc/src/main/cpp/jsonrpcexception.cpp")
set(JSONRPC ${JSONRPC} "${PROJECT_SOURCE_DIR}/third-party/jsonrpc/src/main/cpp/jsonrpcmethod.cpp")
set(JSONRPC ${JSONRPC} "${PROJECT_SOURCE_DIR}/third-party/jsonrpc/src/main/cpp/jsonrpcprivate.cpp")
set(JSONRPC ${JSONRPC} "${PROJECT_SOURCE_DIR}/third-party/jsonrpc/src/main/cpp/jsonrpcstream.cpp")

add_subdirectory(third-party/pybind11)
add_executable(15_pybind 15_pybind/main.cpp)
target_link_libraries(15_pybind pybind11::embed ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${EXTRA_LIBS})

add_executable(24_websocket_srv 24_websocket_srv/main.cpp)
target_link_libraries(24_websocket_srv ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${EXTRA_LIBS})

add_library(jsonrpc STATIC ${JSONRPC})

set(EXTRA_LIBS jsonrpc rt z stdc++)

set(ORANGE_SRCS orange_controller/main.cpp)
set(ORANGE_SRCS ${ORANGE_SRCS} orange_controller/modbus.cpp)
set(GPIO_HL_DIR "${PROJECT_SOURCE_DIR}/third-party/HighLatencyGPIO")
set(ORANGE_SRCS ${ORANGE_SRCS} "${GPIO_HL_DIR}/GPIO.cc")

add_executable(orange_quest ${JSONRPC} ${ORANGE_SRCS} ${INC_HPP} ${INC_H} )
target_link_libraries(orange_quest ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${EXTRA_LIBS} pybind11::embed modbus)
