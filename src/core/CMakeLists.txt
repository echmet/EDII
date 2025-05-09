cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(EDIICore LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
qt_standard_project_setup()

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
    PRIVATE Qt6::Core
    PRIVATE Qt6::Gui
    PRIVATE Qt6::Widgets
    PRIVATE Qt6::Network)

set(CMAKE_CXX_FLAGS
    ${CMAKE_CXX_FLAGS})


if (ECHMET_EDII_USE_DBUS)
    add_definitions("-DECHMET_EDII_IPCINTERFACE_QTDBUS_ENABLED")
    find_package(Qt6DBus REQUIRED)

    set(EDIICore_SRCS
	${EDIICore_SRCS}
        src/dbus/dbusinterface.cpp
        src/dbus/DBusInterfaceAdaptor.cpp
        src/dbus/DBusInterfaceInterface.cpp)

    set(EDIICore_LINK_LIBS
        ${EDIICore_LINK_LIBS}
        Qt6::DBus)

    set(CMAKE_CXX_FLAGS
        ${CMAKE_CXX_FLAGS}
        ${Qt6DBus_EXECUTABLE_COMPILE_FLAGS})
endif ()

qt6_wrap_cpp(EDIICore_MOCS ../../include/plugins/uiplugin.h)

add_executable(EDIICore ${EDIICore_SRCS} ${EDIICore_MOCS})
set_target_properties(EDIICore
	              PROPERTIES LINK_FLAGS ${DEFAULT_SYMVER_LINK})
target_link_libraries(EDIICore ${EDIICore_LINK_LIBS})

install(TARGETS EDIICore
	RUNTIME DESTINATION bin)
