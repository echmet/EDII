cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

if (CMAKE_MAJOR_VERSION LESS 3)
    project(EDIICore LANGUAGES)
else ()
    project(EDIICore LANGUAGES CXX)
endif ()

set(CMAKE_AUTOMOC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network)

include_directories(${INCLUDE_DIRECTORIES}
	            "${CMAKE_CURRENT_SOURCE_DIR}/dbus")

set(EDIICore_SRCS
    src/dataloader.cpp
    src/ipcproxy.cpp
    src/localsocketconnectionhandler.cpp
    src/localsocketipcproxy.cpp
    src/main.cpp
    src/uiplugin.cpp)

if (ECHMET_EDII_USE_DBUS)
    set(EDIICore_SRCS
        ${EDIICore_SRCS}
        src/dbusipcproxy.cpp)
endif ()

set(EDIICore_LINK_LIBS
    PRIVATE Qt5::Core
    PRIVATE Qt5::Gui
    PRIVATE Qt5::Widgets
    PRIVATE Qt5::Network)

set(CMAKE_CXX_FLAGS
    ${CMAKE_CXX_FLAGS}
    ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}
    ${Qt5Gui_EXECUTABLE_COMPILE_FLAGS}
    ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}
    ${Qt5Network_EXECUTABLE_COMPILE_FLAGS})


if (ECHMET_EDII_USE_DBUS)
    add_definitions("-DECHMET_EDII_IPCINTERFACE_QTDBUS_ENABLED")
    find_package(Qt5DBus REQUIRED)

    set(EDIICore_SRCS
	${EDIICore_SRCS}
	src/dbus/dbusinterface.cpp
	src/dbus/DBusInterfaceAdaptor.cpp
	src/dbus/DBusInterfaceInterface.cpp)

    set(EDIICore_LINK_LIBS
	${EDIICore_LINK_LIBS}
	Qt5::DBus)

    set(CMAKE_CXX_FLAGS
	${CMAKE_CXX_FLAGS}
        ${Qt5DBus_EXECUTABLE_COMPILE_FLAGS})
endif ()

qt5_wrap_cpp(EDIICore_MOCS ../../include/plugins/uiplugin.h)

add_executable(EDIICore ${EDIICore_SRCS} ${EDIICore_MOCS})
set_target_properties(EDIICore
	              PROPERTIES LINK_FLAGS ${DEFAULT_SYMVER_LINK})
target_link_libraries(EDIICore ${EDIICore_LINK_LIBS})

install(TARGETS EDIICore
	RUNTIME DESTINATION bin)
