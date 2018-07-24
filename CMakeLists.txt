cmake_minimum_required (VERSION 2.6)
project (glua)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -Werror -Wpedantic -pedantic-errors -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-weak-vtables -Wno-undefined-func-template -MD -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-exit-time-destructors -Wno-global-constructors -Wno-error=deprecated-declarations -Wno-switch")

set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -gdwarf-2 -D_GLIBCXX_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -g -gdwarf-2 -DNDEBUG")

set(SOURCE_FILES
    inc/Exceptions.h
    inc/FileUtil.h src/FileUtil.cpp
    inc/ICallable.h src/ICallable.cpp
    inc/Glua.h inc/Glua.tcc src/Glua.cpp
    inc/GluaCallable.h inc/GluaCallable.tcc
)

include_directories("${PROJECT_SOURCE_DIR}/inc")
add_library(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE /usr/include/luajit-2.1)

### EXAMPLE PROJECT ###
project (libglua-examples)

include_directories("${PROJECT_SOURCE_DIR}/inc")
add_executable(libglua-examples
    src/examples/examples.cpp
)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE /usr/include/luajit-2.1)

target_link_libraries(${PROJECT_NAME} PRIVATE glua luajit-5.1)
#target_link_libraries(${PROJECT_NAME} PRIVATE glua lua)
