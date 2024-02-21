@ECHO OFF
mkdir build_bat_debug >NUL
cd build_bat_debug
cmake -DCMAKE_C_COMPILER=$(clang-cl) -DCMAKE_CXX_COMPILER=$(clang-cl) -DCMAKE_CONFIGURATION_TYPES="Debug" -T ClangCL -DLUA_INCLUDE_PATH=..\dependencies\luajit\src -DLIBLUA=..\dependencies\luajit\src\lua51.lib -DSPIDERMONKEY_INCLUDE_PATH=C:\mozilla-source-115.0.3esr\firefox-115.0.3\obj-opt-x86_64-pc-windows-msvc\dist\include -DLIBSPIDERMONKEY=..\mozjs-115.lib;..\mozglue.lib ..
cmake --build .
copy Debug\libglua-examples.exe ..\ >NUL
cd ..