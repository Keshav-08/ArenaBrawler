cmake_minimum_required(VERSION 3.20)
project(ArenaBrawler CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---------------------------------------------------------------------------
# raylib via FetchContent
# ---------------------------------------------------------------------------
include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# raylib 5.0's own CMakeLists.txt declares `cmake_minimum_required(3.0)`,
# which newer CMake refuses outright; this lets it configure under the
# caller's (much newer) CMake without touching raylib's source tree.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

# ---------------------------------------------------------------------------
# Game executable
# ---------------------------------------------------------------------------
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_executable(ArenaBrawler ${GAME_SOURCES})

target_include_directories(ArenaBrawler PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ArenaBrawler PRIVATE raylib)

if(APPLE)
    target_link_libraries(ArenaBrawler PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")

    # Workaround for CommandLineTools installs whose bundled libc++ headers
    # (usr/include/c++/v1 next to the clang binary) are an incomplete stub —
    # only __cxx_version present, no <cmath>/<string>/etc. When that happens,
    # fall back to the full libc++ header set shipped inside the active SDK.
    if(NOT EXISTS "/Library/Developer/CommandLineTools/usr/include/c++/v1/cmath")
        execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE APPLE_SDK_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(EXISTS "${APPLE_SDK_PATH}/usr/include/c++/v1/cmath")
            message(STATUS "Toolchain libc++ headers look incomplete; falling back to SDK headers at ${APPLE_SDK_PATH}/usr/include/c++/v1")
            target_compile_options(ArenaBrawler PRIVATE -isystem "${APPLE_SDK_PATH}/usr/include/c++/v1")
        endif()
    endif()
endif()

if(MSVC)
    target_compile_options(ArenaBrawler PRIVATE /W4)
else()
    target_compile_options(ArenaBrawler PRIVATE -Wall -Wextra)
endif()
