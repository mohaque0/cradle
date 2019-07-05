# Cradle
A modern C++ build system.

Cradle is a tool for building projects using the components already available on your system if you're building C++ projects. The configuration file is written in C++ and compiled into a binary that builds your project, offering the full power of the C++ language to describe your builds and minimal external tooling.

# Example
This is an example of the configuration to build a simple project that consists of a static library under `test/lib` and other source files under `test/main`.
```cpp
#include <cradle.hpp>

using namespace cradle;

build_config {
	auto conan = conan::conan_install()
			.name("conan")
			.pathToConanfile(".")
			.setting(platform::os::is_windows() ? "compiler.runtime=MT" : "")
			.build();

	auto lib = cpp::static_lib()
			.name("static_lib")
			.sourceFiles(io::FILE_LIST, io::files("lib", ".*.cpp"))
			.includeSearchDirs({"."})
			.includeSearchDirs(conan::INCLUDEDIRS, conan)
			.build();

	auto exe = cpp::exe()
			.name("test_exec")
			.sourceFiles(io::FILE_LIST, io::files("main", ".*.cpp", ".*/build.cpp"))
			.includeSearchDirs(io::FILE_LIST, listOf(io::FILE_LIST, {"."}))
			.linkLibrary(cpp::LIBRARY_NAME, lib)
			.linklibrarySearchPath(cpp::LIBRARY_PATH, lib)
			.linkLibrary(conan::LIBS, conan)
			.linklibrarySearchPath(conan::LIBDIRS, conan)
			.build();
}
```

Then one simply builds the builder as a single source (`build.cpp`) and header file (`cradle.hpp`). For example:
```sh
${CXX} build.cpp -I<path to folder containing cradle.hpp> -std=c++14 -g -o cradle.out
```
or on Windows (MSVC):
```bat
cl build.cpp /I<path to folder containing cradle.hpp> /Fecradle.exe
```

Then one executes the builder with the target name as an argument:
```
./cradle test_exec
```

# Building Cradle
Cradle is written as separate header files found under `includes` that are collected into a single `build/includes/cradle.hpp` file by running `compile.py`. Including this single `cradle.hpp` file in the `build.cpp` configuration will allow you to use cradle.
