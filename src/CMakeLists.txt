add_library(lab1_core
    cardinal.cpp
)

target_include_directories(lab1_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(lab1_cli
    main.cpp
)
target_link_libraries(lab1_cli PRIVATE lab1_core)

find_package(Qt6 COMPONENTS Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(lab1_gui
    gui/main.cpp
    gui/mainwindow.cpp
    gui/mainwindow.h
)

target_include_directories(lab1_gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lab1_gui PRIVATE lab1_core Qt6::Widgets)
