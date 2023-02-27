# Building C++ Files

To avoid having to recompile programs manually using commands in the terminal, we are using makefiles to simplify the build process. Below is a quick overview of how to use Makefiles to compile code.

## CMake

We are using CMake, which is a utility that makes the building and linking process less explicit and easier to read. The code for CMake is found in the "CMakeLists.txt" file in a directory, which generally compiles all of the code in that directory. Before we start though, you may want to create a `build` subdirectory in the directory where the code is, so that the build files don't clutter that directory. 

CMakeLists.txt does not actually do the compiling, it instead generates a Makefile to actually perform the commands. To do this, execute `cmake <path-to-directory-with-cmakelists>`, with the full command being `cmake ..` if in a build subdirectory and `cmake .` if in the current directory. This will generate a Makefile to compile the code. 

## Makefiles

The Makefile can be used to compile individual executables or all of the them at once. To make an individual executable, run `make <executable-name>`, where the executable name can be found by looking at the `add_executable` command in CMakeLists.txt. To make all of the executables, just run `make` or `make all`. 

Once the executable is created, it can be run with `./executable-name` in the directory where the executable is. 