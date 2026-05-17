cmake_minimum_required(VERSION 3.16)

project(AstroAnnotationStudio VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network)

add_executable(AstroAnnotationStudio
    src/main.cpp
    src/applogger.cpp
    src/applogger.h
    src/apptheme.cpp
    src/apptheme.h
    src/mainwindow.cpp
    src/mainwindow.h
    src/imageview.cpp
    src/imageview.h
    src/detectionrectitem.cpp
    src/detectionrectitem.h
    src/dbmigrate.cpp
    src/dbmigrate.h
    src/catalogclient.cpp
    src/catalogclient.h
    src/imageanalyzer.cpp
    src/imageanalyzer.h
    src/llmclient.cpp
    src/llmclient.h
    src/skyfielddialog.cpp
    src/skyfielddialog.h
    ui/mainwindow.ui
    ui/dialog.ui
)

target_include_directories(AstroAnnotationStudio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

target_link_libraries(AstroAnnotationStudio PRIVATE
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
)

add_custom_command(TARGET AstroAnnotationStudio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/schema.sql
        $<TARGET_FILE_DIR:AstroAnnotationStudio>/schema.sql
)
