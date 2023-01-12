@echo off
if not exist build\win_x64 (
    mkdir build\win_x64
)
cd build\win_x64
cmake ..\..\Code -DArch=x64 -DTarget=Windows -DCMAKE_BUILD_TYPE=Debug -DMAPLE_OPENGL=OFF -DMAPLE_VULKAN=ON -DENGINE_AS_LIBRARY=ON -DDISABLE_FILESYSTEM=ON -DDISABLE_TESTS=OFF -DBUILD_STATIC=ON
cd ..\..\
pause