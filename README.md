Introduction
---

libHPCS is a simple library that can read electropherogram and chromatogram data written by HP (Agilent) ChemStation software. Information about the format of the data files were obtained through reverse-engineering. Author of the library puts no guarantee on the correctness of the data or on the ability of libHPCS to read a specific datafile. It is strongly recommended that users of the library verify correctness of the data returned by the library.

Build
---

#### Linux

libHPCS can be built on Linux systems with [CMake](https://cmake.org) build system. To do so, cd to the directory with libHPCS source code and run the following:

	mkdir build
	cd build
	cmake .. -DCMAKE_BUILD_TYPE=Release
	make

libHPCS makes use of the [ICU library](http://site.icu-project.org). ICU 52 or newer (including its development packages, if shipped separately) has to be installed in order to build libHPCS.

#### Other UNIX systems

libHPCS should be buildable on other UNIX systems as well, either manually or through CMake if it is available. However, this has not been tested.

#### Windows

Project for Microsoft Visual Studio 2013 is provided; see the **VS2013** subdirectory. libHPCS uses Windows' own facilities to handle character encodings, ICU library is not required.

Usage
---

Simple testing tool **test_tool.c** is provided to introduce the library's API. Publicly exported functions and data structures are defined in **include/libHPCS.h** header file. Please note that libHPCS allocates memory for it's data structures by itself. The provided **free()** functions shall be used to reclaim the memory.

Reporting bugs and incompatibilities
---

The author of the library has access only to a subset of Agilent instruments and ChemStation software versions. If you wish to extend libHPCS's support for a new file format, you may get in touch with the author.
