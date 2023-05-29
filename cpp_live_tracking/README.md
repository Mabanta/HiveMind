# HiveMind
Repository for GEMSTONE team Hive Mind

## Getting C++ Working on Mac

1. The recommended toolchain is the standard toolchain provided by Apple with XCode. To install this, run:
```
xcode-select --install
```

2. We also require cmake to build applications. The easiest way to install cmake is via Homebrew. Run:
```
brew install cmake
```

3. Download DV: 
Go to this link, download
https://inivation.gitlab.io/dv/dv-docs/docs/getting-started.html

4. Next, setup DV with these commands:
```
brew tap inivation/inivation

brew install libcaer --with-libserialport --with-opencv

brew install dv-runtime
```


5. Then, checkout to the cpp_live_tracking branch and go to from_prof_horiuchi/build directory.
```
git checkout cpp_live_tracking

cd from_prof_horiuchi

cd build
```
7. Run
```
cmake ..

cmake —-build .
```

6. You should see a .exe file that was created

## Running DV Python
1. In your terminal download DV Python library by running
```
pip3 install dv
```
2. Then download open-cv library by running
```
pip3 install opencv-python
```
4. In Hivemind directory terminal, run
```
Pyhton3 <filename>
```


# Sign when you have Git Working!
Matthew Lynch
Daniel was here.
Stefan Traska

# Building C++ Files

To avoid having to recompile programs manually using commands in the terminal, we are using makefiles to simplify the build process. Below is a quick overview of how to use Makefiles to compile code.

## CMake

We are using CMake, which is a utility that makes the building and linking process less explicit and easier to read. The code for CMake is found in the "CMakeLists.txt" file in a directory, which generally compiles all of the code in that directory. Before we start though, you may want to create a `build` subdirectory in the directory where the code is, so that the build files don't clutter that directory. 

CMakeLists.txt does not actually do the compiling, it instead generates a Makefile to actually perform the commands. To do this, execute `cmake <path-to-directory-with-cmakelists>`, with the full command being `cmake ..` if in a build subdirectory and `cmake .` if in the current directory. This will generate a Makefile to compile the code. 

## Makefiles

The Makefile can be used to compile individual executables or all of the them at once. To make an individual executable, run `make <executable-name>`, where the executable name can be found by looking at the `add_executable` command in CMakeLists.txt. To make all of the executables, just run `make install` or `make install all`. 

Once the executable is created, it can be run with `./executable-name` in the directory where the executable is. 

## DV-Modules

The `make install` or `make install <module name>` should compile the module and install it in the appropriate directory. From there, DV can be opened with `sudo dv-gui &`. The "structure" tab allows you to add an configure modules. Select "Add Module" and look for a module with prefix "user_". If it doesn't appear, select "Modify module search path" to select the folder where the module appears (this should be in the output of make install). Then drag the event input from the "Capture" module to input of our module and the output frames to "Visualize in GUI". Then select the play button and return to the "Output" tab. Once data from a camera or file is given, the output from the module should show.   
