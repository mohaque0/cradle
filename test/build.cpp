#include <builder.hpp>

using namespace cradle;

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
