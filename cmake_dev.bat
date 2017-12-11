mkdir build_msvc
cd build_msvc

cmake .. -G "Visual Studio 15 2017 Win64" -DPROJECT_ENABLE_UNITTEST=ON -DPROJECT_ENABLE_SAMPLE=ON