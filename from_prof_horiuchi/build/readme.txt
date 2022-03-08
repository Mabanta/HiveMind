This is the build folder for Dr. Horiuchi's code. It will store all of the 
files that CMake uses when compiling. 

To compile using CMake, run "cmake .." and then "make -j2 -s", which will
create the shared object files. Then run "sudo make install" to install 
the files in dv/modules