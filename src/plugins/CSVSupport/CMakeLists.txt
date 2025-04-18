cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(libCSVSupport LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

add_definitions("-DCSVSUPPORT_LIBRARY")

set(CSVSupport_SRCS
    src/csvsupport.cpp
    src/csvfileloader.cpp
    src/malformedcsvfiledialog.cpp
    src/loadcsvfiledialog.cpp)

set(CMAKE_CXX_FLAGS
    ${CMAKE_CXX_FLAGS})

set(CSVSupport_LINK_LIBS
    PRIVATE Qt6::Core
    PRIVATE Qt6::Gui
    PRIVATE Qt6::Widgets)

if (WIN32)
    set(CSVSupport_LINK_LIBS
        ${CSVSupport_LINK_LIBS}
        user32)
endif ()

add_library(CSVSupport SHARED ${CSVSupport_SRCS})
set_target_properties(CSVSupport
                      PROPERTIES VERSION 0.1
                                 SOVERSION 0.1
                                 LINK_FLAGS ${DEFAULT_SYMVER_LINK})
target_link_libraries(CSVSupport ${CSVSupport_LINK_LIBS})

if (WIN32)
    install(TARGETS CSVSupport
            RUNTIME DESTINATION bin/plugins
            ARCHIVE DESTINATION bin/plugins)
else ()
    install(TARGETS CSVSupport
            LIBRARY DESTINATION bin/plugins)
endif ()

if (WIN32)
    if (MINGW OR MSYS)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libCSVSupport.lib" DESTINATION bin/plugins RENAME CSVSupport.lib)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libCSVSupport.dll" DESTINATION bin/plugins RENAME CSVSupport.dll)
    endif ()
endif ()
