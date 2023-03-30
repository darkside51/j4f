cmake_minimum_required(VERSION 3.17.2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT (j4f_PLATFORM STREQUAL "WINDOWS"))
	set(CMAKE_CXX_FLAGS_DEBUG "-g")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
	message("engine cmake flags (debug+release) configured")
endif ()

if (CMAKE_BUILD_TYPE MATCHES Debug)
	message("debug_mode")
	add_definitions(-D_DEBUG)
else()
	message("release_mode")
endif(CMAKE_BUILD_TYPE MATCHES Debug)

include (${CMAKE_CURRENT_SOURCE_DIR}/../Tools/cmake/utils.cmake)

project(engine)

set(ENGINE_SRC_DIRS)
set(ENGINE_HEADERS)
set(ENGINE_SOURCES)

if (STATISTIC_ENABLED)
	add_definitions(-DENABLE_STATISTIC)
endif()

if (PROFILER_ENABLE)
	add_definitions(-DPROFILER_ENABLE)
endif()

if (GPU_DEBUG_MARKER_ENABLED)
	add_definitions(-DGPU_DEBUG_MARKER_ENABLED)
endif()

if (GPU_VALIDATION_ENABLED)
	add_definitions(-DGPU_VALIDATION_ENABLED)
endif()

add_definitions(-Dj4f_PLATFORM_${j4f_PLATFORM})

if (j4f_PLATFORM STREQUAL "WINDOWS")
	option(GLFW_PLATFORM "Use glfw window" ON)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR=1)
    add_definitions(-DGLFW_EXPOSE_NATIVE_WIN32)
	add_definitions(-DNOMINMAX)
elseif (j4f_PLATFORM STREQUAL "LINUX")
	option(GLFW_PLATFORM "Use glfw window" ON)
	add_definitions(-DVK_USE_PLATFORM_XCB_KHR=1)
#	add_definitions(-DVK_USE_PLATFORM_XLIB_KHR=1)
#	add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR=1)
	add_definitions(-DGLFW_EXPOSE_NATIVE_X11)
else()
	option(GLFW_PLATFORM "Use glfw window" OFF)
endif()

set(
	ENGINE_SRC_DIRS ${ENGINE_SRC_DIRS}
	
	Core
	Core/Math
	Core/Memory
	Core/Threads

	Device

	Graphics
	Graphics/Animation
	Graphics/Vulkan
	Graphics/Vulkan/spirv
	Graphics/Vulkan/spirv/include
	Graphics/Vulkan/spirv/include/spirv
	Graphics/Vulkan/spirv/include/spirv/unified1
	Graphics/Texture
	Graphics/Scene
	Graphics/Scene/LOD
	Graphics/Features
	Graphics/Features/Shadows
	Graphics/Render
	Graphics/Mesh
	Graphics/Text
	Graphics/Plain
	Graphics/UI
	Graphics/UI/ImGui

	Events
	File
	Log
	Time
	Input
	ECS

	Platform

	Utils
	Utils/Debug
	Utils/Json

	../3rd_party/half_float
	../3rd_party/imgui
)

if (GLFW_PLATFORM)
	set(
		ENGINE_SRC_DIRS ${ENGINE_SRC_DIRS}
		Device/GLFW
		Platform/GLFW
	)
else()
	message(FATAL_ERROR "no set correct platform directory!")
endif()

####### для каждого заданного пути
foreach (DIR ${ENGINE_SRC_DIRS})
	message("DIR:" ${DIR})

	# собираем список файлов (не рекурсивно, не GLOB_RECURSE !)
	# можно еще обобщить, задавая список нужных расширений

	file(GLOB
		HEADERS
		${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/*.h
	)

	file(GLOB
		SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/*.cpp
	)

	list(APPEND ENGINE_HEADERS ${HEADERS})
	list(APPEND ENGINE_SOURCES ${SOURCES})

endforeach(DIR)

list(REMOVE_DUPLICATES ENGINE_HEADERS)
list(REMOVE_DUPLICATES ENGINE_SOURCES)
#######

#######
if (j4f_PLATFORM STREQUAL "WINDOWS")
	foreach (source IN LISTS ENGINE_SOURCES)
		get_filename_component(source_path "${source}" PATH)
		string(REPLACE "/" "\\" source_path_msvc "${source_path}")
		string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/ "Engine/" source_path_msvc "${source_path}")
		source_group("${source_path_msvc}" FILES "${source}")
	endforeach()

	foreach (source IN LISTS ENGINE_HEADERS)
		get_filename_component(source_path "${source}" PATH)
		string(REPLACE "/" "\\" source_path_msvc "${source_path}")
		string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/ "Engine/" source_path_msvc "${source_path}")
		source_group("${source_path_msvc}" FILES "${source}")
	endforeach()
endif()
#######

#### vulkan lib
if (j4f_PLATFORM STREQUAL "WINDOWS")
set(
	USED_FRAMEWORKS
	general $ENV{VULKAN_SDK}/Lib/vulkan-1.lib
)
elseif (j4f_PLATFORM STREQUAL "LINUX")
#add_library(vulkan_lib SHARED IMPORTED)
#set_property(TARGET vulkan_lib PROPERTY IMPORTED_LOCATION $ENV{VULKAN_SDK}/lib/libvulkan.so)
#target_link_libraries (
#        ${PROJECT_NAME}
#        vulkan_lib
#)

set(
	USED_FRAMEWORKS
	general $ENV{VULKAN_SDK}/lib/libvulkan.so
)
endif()

add_library(
	${PROJECT_NAME}
	STATIC
	${ENGINE_SOURCES}
	${ENGINE_HEADERS}
)

#### 3rd_party libraries
# freetype library
message("freetype library begin")
add_3rdparty_project (${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/freetype ${CMAKE_BINARY_DIR}/3rd_party/freetype)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/freetype/include)
set(FREETYPE_LIB freetype)
message("freetype library end")
# freetype library

# fmt library
message("fmt library begin")
add_3rdparty_project (${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/fmt ${CMAKE_BINARY_DIR}/3rd_party/fmt)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/fmt/include)
set (FMT_LIB fmt)
message("fmt library end")
# fmt library

# webp library
message("webp library begin")
add_3rdparty_project (${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/libwebp-1.3.0 ${CMAKE_BINARY_DIR}/3rd_party/libwebp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/libwebp-1.3.0/src/webp)
set(WEBP_LIB webp)
message("webp library end")
# webp library

# libpng library
#message("libpng library begin")
#add_3rdparty_project (${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/libpng16 ${CMAKE_BINARY_DIR}/3rd_party/libpng)
#target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/libpng16)
#set(PNG_LIB png_static)
#message("libpng library end")
# libpng library

target_link_libraries (
	${PROJECT_NAME}
	${USED_FRAMEWORKS}
	${FREETYPE_LIB}
	${FMT_LIB}
	${WEBP_LIB}
	#${PNG_LIB}
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/glm)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/imgui)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/stb_image)

#### platform dependency
if (GLFW_PLATFORM)
	# glfw library
	message("glfw library begin")
	add_3rdparty_project (${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/glfw_3.3.8 ${CMAKE_BINARY_DIR}/3rd_party/glfw)
	target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/glfw_3.3.8/include)
	set(GLFW_LIB glfw)
	message("glfw library end")
	# glfw library

	target_link_libraries (
		${PROJECT_NAME}
		${GLFW_LIB}
	)
	target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Platform/GLFW)
endif()

#### vulkan includes
if (j4f_PLATFORM STREQUAL "WINDOWS")
	target_include_directories(${PROJECT_NAME} PUBLIC $ENV{VULKAN_SDK}/Include)
elseif (j4f_PLATFORM STREQUAL "LINUX")
	target_include_directories(${PROJECT_NAME} PUBLIC $ENV{VULKAN_SDK}/include)
endif()

message(STATUS "!!!!!engine generate success!!!!!")