cmake_minimum_required(VERSION 3.19)
project(qmsc_parser LANGUAGES CXX)


if(WIN32)
    set(EXE_EXT ".exe")
endif()

# re2c
macro(RE2C_TARGET Input Output)
    add_custom_command(OUTPUT ${Output}
        COMMAND bin/re2c${EXE_EXT} -s -b -Wno-undefined-control-flow
                -Ibin/include -o ${CMAKE_CURRENT_BINARY_DIR}/${Output} ${Input}
        DEPENDS ${Input}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "[RE2C] Generating scanner")
endmacro()

# lemon
macro(LEMON_TARGET Input Output)
    file(RELATIVE_PATH OUTDIR ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
    add_custom_command(OUTPUT ${Output}
        COMMAND bin/lemon${EXE_EXT} -Tbin/lempar.c -Pgenerated_ -C -d${OUTDIR} ${Input}
        DEPENDS ${Input} bin/lempar.c  # Add lempar.c dependency
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "[LEMON] Generating parser")
endmacro()

RE2C_TARGET(lexer.l generated_lexer.cpp)
LEMON_TARGET(parser.y generated_parser.cpp)

add_library(qmsc_parser STATIC
    ast.h
    lexer.h
    lexer.cpp
    parser.h
    parser.cpp
    visitor.h
    visitor_default.h
    visitor_default.cpp
    visitor_print.h
    visitor_print.cpp
    generated_lexer.cpp
    generated_parser.cpp
)

target_link_libraries(qmsc_parser PRIVATE common_flags)

set_source_files_properties(generated_parser.cpp PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter;-Wno-unused-variable;-Wno-type-limits")
#set_source_files_properties(parser.c PROPERTIES LANGUAGE CXX)

target_include_directories(qmsc_parser PUBLIC .)
target_include_directories(qmsc_parser PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

add_executable(qmsc_test main.cpp)
target_link_libraries(qmsc_test PRIVATE common_flags qmsc_parser)

#add_executable(qmsc_lemon bin/src_lemon.c)
#target_link_libraries(qmsc_lemon PRIVATE -O3 -static)
