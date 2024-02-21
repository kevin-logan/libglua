cmake_minimum_required (VERSION 3.5)

### EXAMPLE PROJECT ###
project (libglua-examples)

set(LUA_INCLUDE_PATH "/usr/include/luajit-2.1" CACHE PATH "User specified lua include path.")
set(LIBLUA "/lib64/libluajit-5.1.so" CACHE FILEPATH "User specified lua library location.")

set(SPIDERMONKEY_INCLUDE_PATH "/usr/include/" CACHE PATH "User specified spidermonkey include path.")
set(LIBSPIDERMONKEY "/lib64/mozjs-125a1.so" CACHE FILEPATH "User specified spidermonkey library location.")

add_executable(libglua-examples
    src/examples.cpp
)

set(DEPENDENCIES ${LIBLUA} ${LIBSPIDERMONKEY})

if(UNIX)
    set(DEPENDENCIES ${DEPENDENCIES} m dl)
endif()

target_include_directories(libglua-examples SYSTEM PRIVATE ${LUA_INCLUDE_PATH} ${SPIDERMONKEY_INCLUDE_PATH})
target_include_directories(libglua-examples PRIVATE  ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(libglua-examples PRIVATE ${DEPENDENCIES})
target_compile_features(libglua-examples PRIVATE cxx_std_23)

if(UNIX)
    target_compile_options(libglua-examples PRIVATE -Wall -Wextra -Werror)
else()
    target_compile_options(libglua-examples PRIVATE /W4 /WX)
endif()
