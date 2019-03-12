CC=clang
CXX=clang++

debug:
	mkdir -p build_debug && \
	cd build_debug && \
	cmake -DCMAKE_C_COMPILER=$(CC) -DCMAKE_CXX_COMPILER=$(CXX) -DCMAKE_BUILD_TYPE="Debug" -DCMAKE_CXX_FLAGS="-D_GLIBCXX_DEBUG" .. && \
	cmake --build . && \
	cp libglua-examples ../ && \
	cd ..

release:
	mkdir -p build_release && \
	cd build_release && \
	cmake -DCMAKE_C_COMPILER=$(CC) -DCMAKE_CXX_COMPILER=$(CXX) -DCMAKE_BUILD_TYPE="Release" .. && \
	cmake --build . && \
	cp libglua-examples ../ && \
	cd ..

clean:
	rm -rf build_debug build_release libglua-examples