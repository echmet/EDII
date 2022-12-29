cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(libEZChromSupport LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_definitions("-DEZCHROM_SUPPORT")

include_directories("${LIBEZFISH_DIR}/include")

find_library(LIBEZFISH ezfish PATHS "${LIBEZFISH_DIR}/lib")

set(EZChromSupport_SRCS
    src/ezchromsupport.cpp
    src/ui/selectchannelsdialog.cpp)

set(EZChromSupport_LINK_LIBS
    PRIVATE Qt5::Core
    PRIVATE Qt5::Gui
    PRIVATE Qt5::Widgets
    PRIVATE ${LIBEZFISH})

link_directories(${LINK_DIRECTORIES} "${LIBEZFISH_DIR}/lib")

if (WIN32)
    set(EZChromSupport_LINK_LIBS
        ${EZChromSupport_LINK_LIBS}
        user32)
endif ()

add_library(EZChromSupport SHARED ${EZChromSupport_SRCS})
set_target_properties(EZChromSupport
                      PROPERTIES VERSION 0.1
                                 SOVERSION 0.1
                                 LINK_FLAGS ${DEFAULT_SYMVER_LINK})
target_link_libraries(EZChromSupport ${EZChromSupport_LINK_LIBS})

if (WIN32)
    install(TARGETS EZChromSupport
            RUNTIME DESTINATION bin/plugins
            ARCHIVE DESTINATION bin/plugins)
else ()
    install(TARGETS EZChromSupport
            LIBRARY DESTINATION bin/plugins)
endif ()

if (WIN32)
    if (MINGW OR MSYS)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libEZChromSupport.lib" DESTINATION bin/plugins RENAME EZChromSupport.lib)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libEZChromSupport.dll" DESTINATION bin/plugins RENAME EZChromSupport.dll)
    endif ()
endif ()
