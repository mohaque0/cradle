BUILD_DIR=build

.PHONY:
test: test.out
	${BUILD_DIR}/test.out main

test.out: test/build.cpp ${BUILD_DIR}/includes/builder.hpp
	mkdir -p ${BUILD_DIR}
	${CXX} test/build.cpp -I${BUILD_DIR}/includes -std=c++14 -g -o ${BUILD_DIR}/test.out

${BUILD_DIR}/includes/builder.hpp: $(wildcard includes/*.hpp)
	./compile.py

.PHONY:
run:
	./build/main

clean:
	rm -rf build
