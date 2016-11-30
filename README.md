# FIFOBuff
This project contains two FIFO buffer C++ template classes: `FIFOBuff` and `FIFOBuff_TS` (both in the `fifobuff.hpp` file).  
The `FIFOBuff` class is suitable for single-threaded environments.  The `FIFOBuff_TS` class is a thread-safe wrapper of 
`FIFOBuff` for an RTOS enabled, multi-threaded environment.  The interface/methods of `FIFOBuff_TS` are 
almost identical to `FIFOBuff`.   Some simple unit tests that utilize the "Google Test" C++ test
framework are also provided in `fifobuff_test.cpp`.

# Building and Running
A CMake file (`CMakeLists.txt`) is provided and, if you're so inclined, you can build and run the unit tests.  
**Disclaimer:** I've only tested this on a single iMac, so there maybe some gotchas with the build process.  
To build and run, clone this repo or download/extract the project.  From the
project directory, the following will hopefully build and run some simple unit test using the FIFO buffer.

```
> mkdir build
> cd build
> cmake ..
> make
> ./fifobuff_test
```

When `cmake ..` is run, it will/should download the Google Test framework from GitHub (this may take a
moment).  The GT source and binaries are stored in the `build` directory that was 
created with the above commands. 
