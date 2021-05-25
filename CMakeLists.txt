cmake_minimum_required(VERSION 3.17)

project(malt)

set(CMAKE_CXX_STANDARD 14)
add_compile_definitions(FMT_HEADER_ONLY)

# set(CMAKE_VERBOSE_MAKEFILE ON)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 1)
string(TIMESTAMP BUILT_ON)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.75 REQUIRED COMPONENTS program_options)

if (DEFINED FMT6_PATH)
    set(FMT6_INCLUDE_FILES ${FMT6_PATH}/include)
else()
    message(FATAL_ERROR "FMT6_PATH not defined. It must point to the fmt6 library root")
endif()

add_library(
        vdunlib
        vdunlib/parsers/IPv4Parsers.cpp
        vdunlib/parsers/UIntParser.cpp
        vdunlib/unix/SysError.cpp
        vdunlib/utils/config/ParamDescrList.cpp
)
target_include_directories(vdunlib PRIVATE ${FMT6_INCLUDE_FILES})
target_include_directories(vdunlib PRIVATE .)

configure_file(src/Version.hpp.in Version.hpp)

add_executable(
        malt
        src/AppUtils.cpp
        src/AppUtils.hpp
        src/Config.cpp
        src/Config.hpp
        src/IPv4IntfList.cpp
        src/IPv4IntfList.hpp
        src/Main.cpp
        src/Malt.cpp
        src/Malt.hpp
        src/MaltBase.hpp
        src/MaltBeaconHdr.hpp
        src/MaltReceiver.hpp
        src/MaltSender.hpp
        src/OutputHandler.cpp
        src/OutputHandler.hpp
        src/PacketInfo.hpp
        src/ReceiverPolicyRaw.hpp
        src/ReceiverPolicyReg.hpp
        src/RxStats.hpp
        src/TimeoutCounter.hpp
)

if (MONOLITHC)
    message(STATUS "Building a monolithic executable")
    set(CMAKE_EXE_LINKER_FLAGS " -static")
else()
    message(STATUS "Building a dynamic executable")
endif()

target_include_directories(malt PRIVATE .)
target_include_directories(malt PRIVATE ${FMT6_INCLUDE_FILES})
target_include_directories(malt PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(malt PRIVATE Boost::program_options)
target_link_libraries(malt PRIVATE vdunlib)
