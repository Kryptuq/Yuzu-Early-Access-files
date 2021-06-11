include(CheckCXXCompilerFlag)

set(SRCS
	src/AAFilter.cpp
	src/BPMDetect.cpp
	src/cpu_detect_x86.cpp
	src/FIFOSampleBuffer.cpp
	src/FIRFilter.cpp
	src/InterpolateCubic.cpp
	src/InterpolateLinear.cpp
	src/InterpolateShannon.cpp
	src/mmx_optimized.cpp
	src/PeakFinder.cpp
	src/RateTransposer.cpp
	src/SoundTouch.cpp
	src/sse_optimized.cpp
	src/TDStretch.cpp)

if(NOT MSVC)
	check_cxx_compiler_flag(-mmmx SOUNDTOUCH_HAS_MMMX)
	check_cxx_compiler_flag(-msse SOUNDTOUCH_HAS_MSSE)
	if(SOUNDTOUCH_HAS_MMMX)
		set_source_files_properties(src/mmx_optimized.cpp
					    PROPERTIES COMPILE_FLAGS "-mmmx")
	endif()
	if(SOUNDTOUCH_HAS_MSSE)
		set_source_files_properties(src/sse_optimized.cpp
					    PROPERTIES COMPILE_FLAGS "-msse")
	endif()
endif()

add_library(SoundTouch STATIC ${SRCS})
target_include_directories(SoundTouch PUBLIC include PRIVATE src)
