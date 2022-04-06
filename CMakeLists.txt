cmake_minimum_required (VERSION 2.6)
project (glua)

set(LUA_INCLUDE_PATH "/usr/include/luajit-2.1" CACHE PATH "User specified lua include path.")
set(LIBLUA "/lib64/libluajit-5.1.so" CACHE FILEPATH "User specified lua library location.")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCE_FILES
    inc/glua/Exceptions.h
    inc/glua/FileUtil.h src/FileUtil.cpp
    inc/glua/GluaBase.h inc/glua/GluaBase.tcc src/GluaBase.cpp
    inc/glua/GluaBaseHelperTemplates.h inc/glua/GluaBaseHelperTemplates.tcc src/GluaBaseHelperTemplates.cpp
    inc/glua/GluaCallable.h inc/glua/GluaCallable.tcc
    inc/glua/GluaLua.h src/GluaLua.cpp
    inc/glua/GluaManagedTypeStorage.h
    inc/glua/StackPosition.h inc/glua/StackPosition.tcc src/StackPosition.cpp
    inc/glua/ICallable.h src/ICallable.cpp
    inc/glua/StringUtil.h src/StringUtil.cpp
)

add_library(glua ${SOURCE_FILES})
target_compile_features(glua PUBLIC cxx_std_17)
target_include_directories(glua SYSTEM PUBLIC ${LUA_INCLUDE_PATH})
target_include_directories(glua PUBLIC ${PROJECT_SOURCE_DIR}/inc)
target_link_libraries(glua PUBLIC ${LIBLUA})

if(UNIX)
    target_compile_options(glua PRIVATE -Wall -Wextra -Werror)
else()
    target_compile_options(glua PRIVATE /W4 /WX)
endif()


### EXAMPLE PROJECT ###
project (libglua-examples)

add_executable(libglua-examples
    src/examples/examples.cpp
)

set(DEPENDENCIES glua ${LIBLUA})

if(UNIX)
    set(DEPENDENCIES ${DEPENDENCIES} m dl)
endif()

target_include_directories(libglua-examples SYSTEM PRIVATE ${LUA_INCLUDE_PATH})
target_include_directories(libglua-examples PRIVATE ${PROJECT_SOURCE_DIR}/inc)
target_link_libraries(libglua-examples PRIVATE ${DEPENDENCIES})
target_compile_features(libglua-examples PRIVATE cxx_std_17)

if(UNIX)
    target_compile_options(libglua-examples PRIVATE -Wall -Wextra -Werror)
else()
    target_compile_options(libglua-examples PRIVATE /W4 /WX)
endif()
