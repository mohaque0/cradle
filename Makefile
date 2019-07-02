BUILD_DIR=build

.PHONY: test run docs

test: ${BUILD_DIR}/cradle
	${BUILD_DIR}/cradle main

${BUILD_DIR}/cradle: test/build.cpp ${BUILD_DIR}/includes/cradle.hpp
	mkdir -p ${BUILD_DIR}
	${CXX} test/build.cpp -I${BUILD_DIR}/includes -std=c++14 -g -o ${BUILD_DIR}/cradle

${BUILD_DIR}/includes/cradle.hpp: $(wildcard includes/*.hpp)
	./compile.py

run:
	./${BUILD_DIR}/main

clean:
	rm -rf ${BUILD_DIR}

docs:
	mkdir -p ${BUILD_DIR}/docs
	doxygen
