cmake_minimum_required(VERSION 3.16)

project(Celested VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

qt_add_executable(Celested
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/imageview.cpp
    src/imageview.h
    src/detectionrectitem.cpp
    src/detectionrectitem.h
    ui/mainwindow.ui
)

target_include_directories(Celested PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Celested PRIVATE
    Qt6::Widgets
    Qt6::Sql
)

set_target_properties(Celested PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS Celested
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
