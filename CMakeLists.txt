cmake_minimum_required (VERSION 2.6)
project (glua)

find_program(CLANG_TIDY_BIN NAMES "clang-tidy" DOC "clang-tidy binary location")
if(CLANG_TIDY_BIN)
    set(CLANG_TIDY_COMMAND "${CLANG_TIDY_BIN}" "-checks=*,-clang-analyzer-alpha.*,-fuchsia-default-arguments,-google-readability-namespace-comments,-llvm-namespace-comment,-hicpp-no-array-decay,-cppcoreguidelines-pro-bounds-array-to-pointer-decay,-google-default-arguments,-llvm-header-guard,-google-runtime-references,-misc-macro-parentheses" "-header-filter=.*")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
    inc/glua/GluaBase.h inc/glua/GluaBase.tcc
    inc/glua/GluaBaseHelperTemplates.h src/GluaBaseHelperTemplates.cpp
    inc/glua/GluaCallable.h inc/glua/GluaCallable.tcc
    inc/glua/GluaLua.h src/GluaLua.cpp
    inc/glua/GluaManagedTypeStorage.h
    inc/glua/ICallable.h src/ICallable.cpp
    inc/glua/StringUtil.h src/StringUtil.cpp
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

if(CLANG_TIDY_BIN)
    set_target_properties(glua PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

### EXAMPLE PROJECT ###
project (libglua-examples)

include_directories("${PROJECT_SOURCE_DIR}/inc")
add_executable(libglua-examples
    src/examples/examples.cpp
)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${LUA_INCLUDE_PATH})

target_link_libraries(${PROJECT_NAME} PRIVATE m dl glua ${LIBLUA})

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
