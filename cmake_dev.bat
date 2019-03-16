mkdir build_msvc
cd build_msvc

cmake .. -G "Visual Studio 16 2019" -A x64 -DPROJECT_ENABLE_UNITTEST=ON -DPROJECT_ENABLE_SAMPLE=ON