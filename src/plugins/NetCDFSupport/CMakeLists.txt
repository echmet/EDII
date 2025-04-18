cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(libNetCDFSupport LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

add_definitions("-DNETCDFSUPPORT_LIBRARY")

set(NetCDFSupport_SRCS
    src/netcdfsupport.cpp
    src/netcdffileloader.cpp)

set(CMAKE_CXX_FLAGS
    ${CMAKE_CXX_FLAGS})

include_directories("${Qt6Core_INCLUDE_DIRS}"
                    "${Qt6Widgets_INCLUDE_DIRS}"
                    "${LIBNETCDF_DIR}/include")

link_directories(${LINK_DIRECTORIES} "${LIBNETCDF_DIR}/lib")

set(NetCDFSupport_LINK_LIBS
    PRIVATE Qt6::Core
    PRIVATE Qt6::Gui
    PRIVATE Qt6::Widgets
    PRIVATE netcdf)

add_library(NetCDFSupport SHARED ${NetCDFSupport_SRCS})
set_target_properties(NetCDFSupport
                      PROPERTIES VERSION 0.1
                      SOVERSION 0.1
                      LINK_FLAGS ${DEFAULT_SYMVER_LINK})
target_link_libraries(NetCDFSupport ${NetCDFSupport_LINK_LIBS})

if (WIN32)
    install(TARGETS NetCDFSupport
            RUNTIME DESTINATION bin/plugins
            ARCHIVE DESTINATION bin/plugins)
else ()
    install(TARGETS NetCDFSupport
            LIBRARY DESTINATION bin/plugins)
endif ()

if (WIN32)
    if (MINGW OR MSYS)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libNetCDFSupport.lib" DESTINATION bin/plugins RENAME NetCDFSupport.lib)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libNetCDFSupport.dll" DESTINATION bin/plugins RENAME NetCDFSupport.dll)
    endif ()
endif ()
