cmake_minimum_required(VERSION 3.19)
project(qmsc LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    set(RELEASE_OPT -fstack-protector-strong -D_FORTIFY_SOURCE=2)
endif()


add_library(common_flags INTERFACE)
target_compile_options(common_flags INTERFACE
    $<$<CONFIG:Debug>:-g -ggdb -O0>
    $<$<CONFIG:Release>:-O3 ${RELEASE_OPT}>
    -Wall -Wextra -Wpedantic -Werror
    -fPIE
)

#qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES en fr)
qt_standard_project_setup()

add_subdirectory(parser)
add_subdirectory(mmi)

qt_add_executable(qmsc WIN32
    main.cpp
)

qt_add_translations(qmsc qmsc_mmi qmsc_parser
    TS_FILES
      i18n/qmsc_fr_FR.ts
      i18n/qmsc_en_US.ts
)

target_link_libraries(qmsc
    PRIVATE
        common_flags
        qmsc_mmi
        qmsc_parser
)

install(TARGETS qmsc
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET qmsc
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
install(PROGRAMS AppRun DESTINATION .)
