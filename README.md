# HiveMind
Repository for GEMSTONE team Hive Mind

## Getting C++ Working on Mac

1. The recommended toolchain is the standard toolchain provided by Apple with XCode. To install this, run:
```console
xcode-select --install
```

2. We also require cmake to build applications. The easiest way to install cmake is via Homebrew. Run:
```console
brew install cmake
```

3. Download DV: 
Go to this link, download
https://inivation.gitlab.io/dv/dv-docs/docs/getting-started.html

4. Next, setup DV with these commands:
```console

brew tap inivation/inivation

brew install libcaer --with-libserialport --with-opencv

brew install dv-runtime
```


5. Then, 'git checkout' to the cpp_live_tracking branch, go to from_prof_horiuchi/build, and run:
```console

cmake ..

cmake —-build .
```

6. You should see a .exe file that was created
