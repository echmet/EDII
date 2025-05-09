cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(libHPCSSupport LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

add_definitions("-DHPCSSUPPORT_LIBRARY")

set(HPCSSupport_SRCS
    src/chemstationbatchloader.cpp
    src/chemstationbatchloadmodel.cpp
    src/chemstationfileinfomodel.cpp
    src/chemstationfileloader.cpp
    src/hpcssupport.cpp
    src/loadchemstationdatadialog.cpp)

set(CMAKE_CXX_FLAGS
    ${CMAKE_CXX_FLAGS})

set(HPCSSupport_LINK_LIBS
    PRIVATE Qt6::Core
    PRIVATE Qt6::Gui
    PRIVATE Qt6::Widgets
    PRIVATE HPCS)

include_directories("${LIBHPCS_DIR}/include")

link_directories(${LINK_DIRECTORIES} "${LIBHPCS_DIR}/lib")

if (WIN32)
    set(HPCSSupport_LINK_LIBS
        ${HPCSSupport_LINK_LIBS}
        user32)
endif ()

add_library(HPCSSupport SHARED ${HPCSSupport_SRCS})
set_target_properties(HPCSSupport
                      PROPERTIES VERSION 0.1
                      SOVERSION 0.1
                      LINK_FLAGS ${DEFAULT_SYMVER_LINK})
target_link_libraries(HPCSSupport ${HPCSSupport_LINK_LIBS})

if (WIN32)
    install(TARGETS HPCSSupport
            RUNTIME DESTINATION bin/plugins
            ARCHIVE DESTINATION bin/plugins)
else ()
    install(TARGETS HPCSSupport
            LIBRARY DESTINATION bin/plugins)
endif ()

if (WIN32)
    if (MINGW OR MSYS)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libHPCSSupport.lib" DESTINATION bin/plugins RENAME HPCSSupport.lib)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libHPCSSupport.dll" DESTINATION bin/plugins RENAME HPCSSupport.dll)
    endif ()
endif ()
