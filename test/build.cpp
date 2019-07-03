#include <cradle.hpp>

using namespace cradle;

build_config {
    auto lib = cpp::static_lib()
            .name("static_lib")
			.sourceFiles(io::files("lib", ".*.cpp"))
			.includeSearchDirs({"."})
            .build();

    auto exe = cpp::exe()
			.name("test_exec")
			.sourceFiles(io::files("main", ".*.cpp", ".*/build.cpp"))
			.includeSearchDirs(listOf(io::FILE_LIST, {"."}))
            .linkLibraryTasks(lib)
            .build();
}
