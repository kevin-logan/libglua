cmake_minimum_required (VERSION 2.6)
project (glua)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -Weffc++ -Werror -Wpedantic -pedantic-errors -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-weak-vtables -Wno-undefined-func-template -MD -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-exit-time-destructors -Wno-global-constructors -Wno-error=deprecated-declarations -fcolor-diagnostics -fcomment-block-commands=copydoc -Wno-switch -ferror-limit=123")

set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -gdwarf-2 -D_GLIBCXX_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -g -gdwarf-2 -DNDEBUG")

set(SOURCE_FILES
    inc/exceptions.h
    inc/file_util.h src/file_util.cpp
    inc/icallable.h src/icallable.cpp
    inc/luacallable.h src/luacallable.cpp
)

include_directories("${PROJECT_SOURCE_DIR}/inc")
add_library(${PROJECT_NAME} ${SOURCE_FILES})

### EXAMPLE PROJECT ###
project (libglua-example)

include_directories("${PROJECT_SOURCE_DIR}/inc")
add_executable(libglua-example
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE glua lua)
