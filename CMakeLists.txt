cmake_minimum_required(VERSION 3.31)
project(VAudio)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Svg
        REQUIRED)

add_executable(VAudio
        icons.qrc
        main.cpp
        verticalscrollarea.cpp
        verticalscrollarea.h
        mainwindow.cpp
        mainwindow.h
        sinkpanel.cpp
        sinkpanel.h
        addsinkpanel.cpp
        addsinkpanel.h
        inputpanel.cpp
        inputpanel.h
        pipewireapi.cpp
        pipewireapi.h)
target_link_libraries(VAudio
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Svg
)

target_link_libraries(VAudio
        ${PIPEWIRE_LIBRARIES}
)

target_include_directories(VAudio
        PUBLIC
        ${PIPEWIRE_INCLUDE_DIRS}
)

target_compile_options(VAudio
        PUBLIC
        ${PIPEWIRE_CFLAGS_OTHER}
        -fpermissive
)

