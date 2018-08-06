cmake_minimum_required (VERSION 2.6)
project (glua)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

if(NOT "$ENV{LUA_INCLUDE_PATH}" STREQUAL "")
    set(LUA_INCLUDE_PATH $ENV{LUA_INCLUDE_PATH})
else()
    set(LUA_INCLUDE_PATH "/usr/include/luajit-2.1" CACHE STRING "User specified lua include path.")
endif()

if(NOT "$ENV{LIBLUA}" STREQUAL "")
    set(LIBLUA $ENV{LIBLUA})
else()
    set(LIBLUA "luajit-5.1" CACHE STRING "User specified lua library location.")
endif()

option(D_GLIBCXX_DEBUG "Use D_GLIBCXX_DEBUG when building Debug" ON)

set(SOURCE_FILES
    inc/glua/Exceptions.h
    inc/glua/FileUtil.h src/FileUtil.cpp
    inc/glua/ICallable.h src/ICallable.cpp
    inc/glua/Glua.h inc/glua/Glua.tcc src/Glua.cpp
    inc/glua/GluaCallable.h inc/glua/GluaCallable.tcc
)

include_directories("${PROJECT_SOURCE_DIR}/inc")
add_library(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${LUA_INCLUDE_PATH})

target_compile_options(
    ${PROJECT_NAME} PRIVATE
    -std=c++17
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    -pedantic-errors
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-padded
    -Wno-weak-vtables
    -Wno-undefined-func-template
    -MD
    -Wno-gnu-anonymous-struct
    -Wno-nested-anon-types
    -Wno-exit-time-destructors
    -Wno-global-constructors
    -Wno-error=deprecated-declarations
    -Wno-switch
)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    target_compile_options(
        ${PROJECT_NAME} PRIVATE
        -g
        -O0
        -gdwarf-2
    )

    if(D_GLIBCXX_DEBUG)
    target_compile_options(
        ${PROJECT_NAME} PRIVATE
        -D_GLIBCXX_DEBUG
    )
    endif()
else()
    target_compile_options(
        ${PROJECT_NAME} PRIVATE
        -O2
        -g
        -gdwarf-2
        -DNDEBUG
    )
endif()

### EXAMPLE PROJECT ###
project (libglua-examples)

include_directories("${PROJECT_SOURCE_DIR}/inc")
add_executable(libglua-examples
    src/examples/examples.cpp
)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${LUA_INCLUDE_PATH})

target_link_libraries(${PROJECT_NAME} PRIVATE glua ${LIBLUA})

target_compile_options(
    ${PROJECT_NAME} PRIVATE
    -std=c++17
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    -pedantic-errors
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-padded
    -Wno-weak-vtables
    -Wno-undefined-func-template
    -MD
    -Wno-gnu-anonymous-struct
    -Wno-nested-anon-types
    -Wno-exit-time-destructors
    -Wno-global-constructors
    -Wno-error=deprecated-declarations
    -Wno-switch
)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    target_compile_options(
        ${PROJECT_NAME} PRIVATE
        -g
        -O0
        -gdwarf-2
    )

    if(D_GLIBCXX_DEBUG)
    target_compile_options(
        ${PROJECT_NAME} PRIVATE
        -D_GLIBCXX_DEBUG
    )
    endif()
else()
    target_compile_options(
        ${PROJECT_NAME} PRIVATE
        -O2
        -g
        -gdwarf-2
        -DNDEBUG
    )
endif()
