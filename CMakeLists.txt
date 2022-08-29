cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(EDII LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (WIN32)
    option(ECHMET_EDII_USE_DBUS "Use D-Bus interface for IPC" OFF)
	option(ECHMET_EDII_FORCE_WINXP "Define target Windows version to Windows XP")
else ()
    option(ECHMET_EDII_USE_DBUS "Use D-Bus interface for IPC" ON)
endif ()

if (WIN32)
    if (MINGW OR MSYS)
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
            message(FATAL_ERROR "MinGW 10.0 or newer is required to build EDII")
        endif ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wno-unused-function -fvisibility=hidden -ftree-vectorize -g -O2 -msse2")
        add_definitions("-DECHMET_COMPILER_MINGW")
        set(DEFAULT_SYMVER_LINK "-Wl,--default-symver")
    elseif (MSVC)
        add_definitions("-DECHMET_COMPILER_MSVC")
        set(DEFAULT_SYMVER_LINK "")
    else()
        message(FATAL_ERROR "Compiler not recognized or not supported")
    endif()
elseif (UNIX)
    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR
        ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND
            CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
            message(FATAL_ERROR "GCC 10.0 or newer is required to build EDII")
        elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" AND
	        CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
            message(FATAL_ERROR "Clang 8.0 or newer is required to build EDII")
        endif ()

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -Wall -Wextra -pedantic -Wno-unused-function -fvisibility=hidden -g -O2")
        add_definitions("-DECHMET_COMPILER_GCC_LIKE")
        set(DEFAULT_SYMVER_LINK "-Wl,--default-symver")
    endif ()
else()
    message(FATAL_ERROR "Platform not recognized or not supported")
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(src/core)
add_subdirectory(src/plugins)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include" DESTINATION . FILES_MATCHING PATTERN "*.h")
