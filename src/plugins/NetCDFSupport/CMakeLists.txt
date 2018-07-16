cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

if (CMAKE_MAJOR_VERSION LESS 3)
    project(libNetCDFSupport)
else ()
    project(libNetCDFSupport LANGUAGES CXX)
endif ()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)

add_definitions("-DNETCDFSUPPORT_LIBRARY")

set(NetCDFSupport_SRCS
    src/netcdfsupport.cpp
    src/netcdffileloader.cpp)

set(CMAKE_CXX_FLAGS
    ${CMAKE_CXX_FLAGS}
    ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}
    ${Qt5Gui_EXECUTABLE_COMPILE_FLAGS}
    ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS})

include_directories("${Qt5Core_INCLUDE_DIRS}"
                    "${Qt5Widgets_INCLUDE_DIRS}"
                    "${LIBNETCDF_DIR}/include")

link_directories("${LIBNETCDF_DIR}/lib")

set(NetCDFSupport_LINK_LIBS
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    netcdf)

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
