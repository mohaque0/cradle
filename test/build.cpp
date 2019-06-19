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
