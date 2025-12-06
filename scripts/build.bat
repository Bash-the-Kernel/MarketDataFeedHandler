@echo off
echo Building standalone version (no Boost required)...
mkdir build 2>nul
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
cd ..
echo.
echo Build complete! Executables are in build\Release\
