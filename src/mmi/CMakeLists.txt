cmake_minimum_required(VERSION 3.19)
project(qmsc_mmi LANGUAGES CXX)

qt_standard_project_setup()

add_library(qmsc_mmi STATIC
    qmsc_graphics.cpp
    qmsc_graphics.h
    qmsc_main_window.cpp
    qmsc_main_window.h
    qmsc_main_window.ui
    qmsc_child.h
    qmsc_child.cpp
    qmsc_document.h
    qmsc_document.cpp
    qmsc_edit.h
    qmsc_edit.cpp
    qmsc_utils.h
    qmsc_utils.cpp
    qmsc_settings.h
    qmsc_settings.cpp
    qmsc_config_dialog.h
    qmsc_config_dialog.cpp
    qmsc_config_dialog.ui
    qmsc_about_dialog.h
    qmsc_about_dialog.cpp
    qmsc_about_dialog.ui
)

qt_add_resources(qmsc_mmi "images" PREFIX "/"
    FILES
    images/document.png
    images/app.png
    images/logo.png
)

qt_add_resources(qmsc_mmi "msc" PREFIX "/"
    FILES
    msc/default.msc
    msc/anno.msc
    msc/condition.msc
    msc/reference.msc
)

target_include_directories(qmsc_mmi PUBLIC . ../parser)

target_link_libraries(qmsc_mmi
    PRIVATE
        common_flags
    PUBLIC
        Qt6::Core
        Qt6::Widgets
)
install(FILES qmsc.desktop images/app.png DESTINATION .)
