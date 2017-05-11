libHPCS
===

Introduction
---

libHPCS is a simple library that can read electrophoregram and chromatogram data written by HP (Agilent) ChemStation software. Information about the format of the data files were obtained strictly through reverse-engineering. Author of the library puts no guarantee on the correctness of the data or on the ability of libHPCS to read a specific data file. It is strongly recommended that the users of the library verify the correctness of the data returned by the library.

Build
---

#### Linux

libHPCS can be built on Linux systems with [CMake](https://cmake.org) build system. To do so, `cd` to the directory with libHPCS source code and run the following:

	mkdir build
	cd build
	cmake .. -DCMAKE_BUILD_TYPE=Release
	make
	make install

libHPCS makes use of the [ICU library](http://site.icu-project.org). ICU 52 or newer (including its development packages, if shipped separately) has to be installed in order to build libHPCS.

#### Other UNIX systems

libHPCS should be buildable on other UNIX systems as well, either manually or through CMake if it is available. However, this has not been tested.

#### Windows

Project for Microsoft Visual Studio 2013 is provided; see the **VS2013** subdirectory. libHPCS uses Windows' own facilities to handle character encodings, the ICU library is not required.

Usage
---

Simple testing tool `test_tool.c` is provided to demonstrate the library's API and display sample output. The test tool is not built by default; supply `-DBUILD_TEST_TOOL=ON` parameter to CMake if you wish to build the tool along with the library. Publicly exported functions and data structures are defined in `libHPCS.h` header file. Please note that libHPCS allocates memory for its data structures by itself. The provided `hpcs_free_*()` functions shall be used to reclaim the memory.

Reporting bugs and incompatibilities
---

The author of the library has access only to a subset of Agilent instruments and ChemStation software versions. If you wish to extend libHPCS' support for a new file format, you may get in touch with the author.

Licensing
---

libHPCS is distributed under the terms of [The GNU Lesser General Public License v3](https://www.gnu.org/licenses/gpl-3.0.en.html).
