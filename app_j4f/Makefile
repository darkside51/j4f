
win64:
	cmake -S . -B build/win64 \
	-Dj4f_PLATFORM="WINDOWS" \
	-DSTATISTIC_ENABLED=true \
	-DPROFILER_ENABLE=true \
	-DGPU_DEBUG_MARKER_ENABLED=true \
	-DGPU_VALIDATION_ENABLED=true
	
linux:
	cmake -S . -B build/linux \
	-Dj4f_PLATFORM="LINUX" \
	-DSTATISTIC_ENABLED=true \
	-DPROFILER_ENABLE=false \
	-DGPU_DEBUG_MARKER_ENABLED=false \
	-DGPU_VALIDATION_ENABLED=true

clean:
	rm -r build