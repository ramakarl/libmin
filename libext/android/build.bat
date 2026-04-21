batch
@echo off

:: --- INSTALL ANDROID SDK FIRST ---
:: Find Ninja and NDK inside your Android SDK (version number might vary)

set NINJA_EXE=E:/AndroidSDK/cmake/4.1.2/bin/ninja.exe
set NDK=E:/AndroidSDK/ndk/25.2.9519653
set BUILD_DIR=build_android_arm64

:: --- REMOVE OLD BUILD ---
if exist %BUILD_DIR% rd /s /q %BUILD_DIR%
mkdir %BUILD_DIR%
cd %BUILD_DIR%

:: Run CMake using that Ninja
cmake -G "Ninja" ^
  -DCMAKE_MAKE_PROGRAM=%NINJA_EXE% ^
  -DCMAKE_TOOLCHAIN_FILE=%NDK%/build/cmake/android.toolchain.cmake ^
  -DANDROID_ABI=arm64-v8a ^
  -DANDROID_PLATFORM=android-28 ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DENABLE_SHARED=OFF ^
  -DENABLE_STATIC=ON ^
  ..

:: --- RUN THE COMPILER ---
cmake --build .

pause