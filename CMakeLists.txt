cmake_minimum_required(VERSION 3.27)
project(Y_browser)

set(CMAKE_CXX_STANDARD 23)

include_directories(.)

add_executable(Y_browser
        main.cpp
        URL.cpp
        URL.hpp)

# Dependencies

set(DEPENDENCIES_ROOT /home/ykarroum/Dependencies)
## boost
set(BOOST_VERSION 1_84_0)
set(BOOST_ROOT ${DEPENDENCIES_ROOT}/boost_${BOOST_VERSION})
### boost gzip
target_include_directories(Y_browser PUBLIC ${BOOST_ROOT}/boost/iostreams/filter/)
add_library(BoostZip
        ${BOOST_ROOT}/libs/iostreams/src/gzip.cpp
        ${BOOST_ROOT}/libs/iostreams/src/zlib.cpp
)
target_link_libraries(BoostZip -lz)
target_link_libraries(Y_browser BoostZip)
