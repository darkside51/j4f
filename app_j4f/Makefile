
win64:
	cmake -S . -B build/win64 \
	-DSTATISTIC_ENABLED=true \
	-DPROFILER_ENABLE=true \
	-DGPU_DEBUG_MARKER_ENABLED=true \
	-DGPU_VALIDATION_ENABLED=true
	
linux:
	mkdir -p build/linux
	mkdir -p build/linux/debug
	mkdir -p build/linux/release
	python3 j4f/Tools/project/make_linux_params.py
# 	cmake \
# 	-DSTATISTIC_ENABLED=true \
# 	-DPROFILER_ENABLE=false \
# 	-DGPU_DEBUG_MARKER_ENABLED=false \
# 	-DGPU_VALIDATION_ENABLED=true


linux_debug:
	mkdir -p build/linux_debug && cd build/linux_debug && \
	cmake \
	-DSTATISTIC_ENABLED=true \
 	-DPROFILER_ENABLE=false \
 	-DGPU_DEBUG_MARKER_ENABLED=false \
 	-DGPU_VALIDATION_ENABLED=true \
 	-DCMAKE_BUILD_TYPE=Debug ../../ && \
	make -j 8

linux_release:
	mkdir -p build/linux_release && cd build/linux_release && \
	cmake \
	-DSTATISTIC_ENABLED=true \
 	-DPROFILER_ENABLE=false \
 	-DGPU_DEBUG_MARKER_ENABLED=false \
 	-DGPU_VALIDATION_ENABLED=true \
 	-DCMAKE_BUILD_TYPE=Release ../../ && \
	make -j 8

# linux_test:
# 	cmake -S . -B build/linux2 \
#  	-DSTATISTIC_ENABLED=true \
#  	-DPROFILER_ENABLE=false \
#  	-DGPU_DEBUG_MARKER_ENABLED=false \
#  	-DGPU_VALIDATION_ENABLED=true


clean:
	rm -r build
	rm -r .idea