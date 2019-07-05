#include <cradle.hpp>

using namespace cradle;

build_config {
	auto conan = conan::conan_install()
			.name("conan")
			.installFolder("build")
			.pathToConanfile(".")
			.build();

    auto lib = cpp::static_lib()
            .name("static_lib")
			.sourceFiles(io::FILE_LIST, io::files("lib", ".*.cpp"))
			.includeSearchDirs({"."})
            .build();

    auto exe = cpp::exe()
			.name("test_exec")
			.sourceFiles(io::FILE_LIST, io::files("main", ".*.cpp", ".*/build.cpp"))
			.includeSearchDirs(io::FILE_LIST, listOf(io::FILE_LIST, {"."}))
			.linkLibrary(cpp::LIBRARY_NAME, lib)
			.linklibrarySearchPath(cpp::LIBRARY_PATH, lib)
			//.linkLibrary(conan::LIBS, conan)
            .build();
}
