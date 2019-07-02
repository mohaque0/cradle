# Cradle
A modern C++ build system.

Cradle is a tool for building projects using the components already available on your system if you're building C++ projects. The configuration file is written in C++ and compiled into a binary that builds your project, offering the full power of the C++ language to describe your builds and minimal external tooling.

# Example
This is an example of the configuration to build a simple project that consists of a static library under `test/lib` and other source files under `test/main`.
```cpp
#include <builder.hpp>

build_config {
    auto lib = cpp::static_lib()
            .name("static_lib")
            .sourceFiles(io::files("test/lib", ".*.cpp"))
            .includeSearchDirs({"test"})
            .build();

    auto exe = cpp::exe()
            .name("main")
            .sourceFiles(io::files("test/main", ".*.cpp", ".*/build.cpp"))
            .includeSearchDirs(listOf(io::FILE_LIST, {"includes", "test"}))
            .linkLibraryTasks(lib)
            .build();
}
```

Then one simply builds the builder as a single source (`build.cpp`) and header file (`builder.hpp`). For example:
```sh
${CXX} build.cpp -I<path to folder containing builder.hpp> -std=c++14 -g -o builder.out
```
or on Windows (MSVC):
```bat
cl build.cpp /I<path to folder containing builder.hpp> /Febuilder.exe
```

Then one executes the builder with the target name as an argument:
```
./builder main
```

# Building Cradle
Cradle is written as separate header files found under `includes` that are collected into a single `build/includes/cradle.hpp` file by running `compile.py`. Including this single `cradle.hpp` file in the `build.cpp` configuration will allow you to use cradle.
