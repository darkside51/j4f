
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
# 	cmake \
# 	-DSTATISTIC_ENABLED=true \
# 	-DPROFILER_ENABLE=false \
# 	-DGPU_DEBUG_MARKER_ENABLED=false \
# 	-DGPU_VALIDATION_ENABLED=true

clean:
	rm -r build
	rm -r .idea