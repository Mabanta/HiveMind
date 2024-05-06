rm -rf build
mkdir build
cd build
CC=gcc-10 CXX=g++-10 cmake ..
make install all