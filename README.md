# HiveMind
Repository for GEMSTONE team Hive Mind

# Getting C++ Working on Mac

1. Download DV: 
Go to this link, download
https://inivation.gitlab.io/dv/dv-docs/docs/getting-started.html

2. Next, setup DV with these commands:

brew tap inivation/inivation

brew install libcaer --with-libserialport --with-opencv

brew install dv-runtime



3. Then, 'git checkout' to the cpp_live_tracking branch, go to from_prof_horiuchi/build, and run:

cmake ..

cmake —build .

4. You should see a .exe file that was created
