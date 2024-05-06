rm -rf ../cluster/build
mkdir ../cluster/build
cd ../cluster/build
CC=gcc-10 CXX=g++-10 cmake ..
make all
cd ../../object_detection
rm -rf build
mkdir build
cd build
CC=gcc-10 CXX=g++-10 cmake ..
make install all