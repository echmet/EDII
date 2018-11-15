cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

if (CMAKE_MAJOR_VERSION LESS 3)
    project(libASCSupport)
else ()
    project(libASCSupport LANGUAGES CXX)
endif ()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)

add_definitions("-DASCSUPPORT_LIBRARY")

set(ASCSupport_SRCS
    src/ascsupport_handlers.cpp
    src/ascsupport.cpp
    src/availablechannels.cpp
    src/supportedencodings.cpp
    src/ui/commonpropertiesdialog.cpp
    src/ui/pickdecimalpointdialog.cpp
    src/ui/selectchannelsdialog.cpp)

set(CMAKE_CXX_FLAGS
    ${CMAKE_CXX_FLAGS}
    ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}
    ${Qt5Gui_EXECUTABLE_COMPILE_FLAGS}
    ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS})

set(ASCSupport_LINK_LIBS
    PRIVATE Qt5::Core
    PRIVATE Qt5::Gui
    PRIVATE Qt5::Widgets)

if (UNIX)
    add_definitions("-DENCODING_USE_ICU")
    find_package(ICU REQUIRED COMPONENTS uc i18n)
    set(ASCSupport_LINK_LIBS
        ${ASCSupport_LINK_LIBS}
        ICU::uc
        ICU::i18n)
elseif (WIN32)
    add_definitions("-DENCODING_USE_WINAPI")
    set(ASCSupport_LINK_LIBS
        ${ASCSupport_LINK_LIBS}
        user32)
endif ()

add_library(ASCSupport SHARED ${ASCSupport_SRCS})
set_target_properties(ASCSupport
                      PROPERTIES VERSION 0.1
                      SOVERSION 0.1
                      LINK_FLAGS ${DEFAULT_SYMVER_LINK})
target_link_libraries(ASCSupport ${ASCSupport_LINK_LIBS})

if (WIN32)
   install(TARGETS ASCSupport
           RUNTIME DESTINATION bin/plugins
           ARCHIVE DESTINATION bin/plugins)
else ()
    install(TARGETS ASCSupport
            LIBRARY DESTINATION bin/plugins)
endif ()

if (WIN32)
    if (MINGW OR MSYS)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libASCSupport.lib" DESTINATION bin/plugins RENAME ASCSupport.lib)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libASCSupport.dll" DESTINATION bin/plugins RENAME ASCSupport.dll)
    endif ()
endif ()
