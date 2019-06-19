# Cradle
A modern C++ build system.

Cradle is a tool for building projects using the components already available on your system if you're building c++ projects. The configuration file is written in C++ and compiled into a binary that builds your project, offering the full power of the C++ language to describe your builds and minimal external tooling.

# Example
This is an example of the configuration to build a simple project that consists of a static library under `test/lib` and other source files under `test/main`.
```cpp
#include <builder.hpp>

build_config {
	auto lib = cpp::static_lib(
		"static_lib",
		io::files("test/lib", ".*.cpp"),
		listOf(io::FILE_LIST, {"test"})
	);

	auto exe = cpp::exe(
		"main",
		io::files("test/main", ".*.cpp", ".*/build.cpp"),
		listOf(io::FILE_LIST, {"includes", "test"}),
		{lib}
	);
}
```

Then one simply builds the builder as a single source (`build.cpp`) and header file (`builder.hpp`). For example:
```sh
${CXX} build.cpp -I<path to folder containing builder.hpp> -std=c++14 -g -o builder.out
```
or on Windows:
```bat
cl build.cpp /I<path to folder containing builder.hpp> /Febuilder.exe
```

Then one executes the builder with the target name as an argument:
```
./builder main
```

# Building Cradle
Cradle is written as separate header files found under `includes` that are collected into a single `build/includes/builder.hpp` file by running `compile.py`. Including this single `builder.hpp` file in the `build.cpp` configuration is sufficient.
