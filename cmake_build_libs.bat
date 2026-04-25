:: Step 1: Generate CMake project
call cmake -B build/cmake_lib -S src/lib/ -A x64

if ERRORLEVEL 1 (
    echo CMake generation failed. Aborting build.
    exit /b 1
)

:: Step 2: Build only if generation succeeded
call cmake --build build/cmake_lib --config RelWithDebInfo

echo Build succeeded.