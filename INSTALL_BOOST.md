# Installing Boost on Windows

## Option 1: Using vcpkg (Recommended)

1. Install vcpkg if you don't have it:
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

2. Install Boost:
```bash
.\vcpkg install boost-asio:x64-windows
```

3. Build the project with vcpkg toolchain:
```bash
cd C:\Users\jackw\Documents\GitHub\MarketDataFeedHandler
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

## Option 2: Download Prebuilt Boost

1. Download from: https://sourceforge.net/projects/boost/files/boost-binaries/
2. Install to C:\boost
3. Set environment variable: BOOST_ROOT=C:\boost
4. Run build.bat

## Option 3: Build without Boost (Standalone)

Use the standalone version that doesn't require external dependencies.
See: STANDALONE_BUILD.md
