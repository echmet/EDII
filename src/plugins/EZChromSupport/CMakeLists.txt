cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(libEZChromSupport LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (UNIX)
    include(GNUInstallDirs)
elseif (WIN32)
    set(CMAKE_INSTALL_LIBDIR "lib")
    set(CMAKE_INSTALL_INCLUDEDIR "include")
    set(CMAKE_INSTALL_BINDIR "bin")
endif ()

set(LIBEZFISH_DIR
    ""
	CACHE
	PATH
	"Path to EZFish installation directory. Set only of EZFish is not installed system-wide"
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

add_definitions("-DEZCHROMSUPPORT_LIBRARY")

include_directories("${LIBEZFISH_DIR}/${CMAKE_INSTALL_INCLUDEDIR}")

set(EZChromSupport_SRCS
    src/ezchromsupport.cpp
    src/ui/selectchannelsdialog.cpp)

set(EZChromSupport_LINK_LIBS
    PRIVATE Qt5::Core
    PRIVATE Qt5::Gui
    PRIVATE Qt5::Widgets
    PRIVATE ezfish)

link_directories(${LINK_DIRECTORIES} "${LIBEZFISH_DIR}/${CMAKE_INSTALL_LIBDIR}")

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
