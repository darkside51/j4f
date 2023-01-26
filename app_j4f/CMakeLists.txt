cmake_minimum_required(VERSION 3.17.2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(app_j4f)

if(j4f_PLATFORM STREQUAL "WINDOWS")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()

if(STATISTIC_ENABLED)
	add_definitions(-DENABLE_STATISTIC)
endif()

if(PROFILER_ENABLE)
	add_definitions(-DPROFILER_ENABLE)
endif()

if(GPU_DEBUG_MARKER_ENABLED)
	add_definitions(-DGPU_DEBUG_MARKER_ENABLED)
endif()

if(GPU_VALIDATION_ENABLED)
	add_definitions(-DGPU_VALIDATION_ENABLED)
endif()

add_definitions(-Dj4f_PLATFORM_${j4f_PLATFORM})

#собираем engine
add_subdirectory(j4f/Engine)

#собираем приложение 
######################################################################################################
set(APP_RES_DIRS)
set(APP_RESOURCES)

set(
	APP_RES_DIRS ${APP_RES_DIRS}
	application
	application/resources
	application/resources/shaders
)

foreach(DIR ${APP_RES_DIRS})
	message("DIR:" ${DIR})

	file(GLOB
		RESOURCE_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/*.vsh
		${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/*.psh
		${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/*.gsh
	)

	list(APPEND APP_RESOURCES ${RESOURCE_FILES})

endforeach(DIR)

list(REMOVE_DUPLICATES APP_RESOURCES)

set(APP_RESOURCES_DIRECTORY application/resources)

if(j4f_PLATFORM STREQUAL "WINDOWS")
foreach(source IN LISTS APP_RESOURCES)
	get_filename_component(source_path "${source}" PATH)
	string(REPLACE "/" "\\" source_path_msvc "${source_path}")
	string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/${APP_RESOURCES_DIRECTORY}/ "Resources/" source_path_msvc "${source_path}")
	source_group("${source_path_msvc}" FILES "${source}")
endforeach()
endif()
######################################################################################################

add_executable(${PROJECT_NAME} application/src/main.cpp ${APP_RESOURCES})

#подключаем engine
target_link_libraries(${PROJECT_NAME} j4f_engine)

include_directories(j4f/)

# копируем ресурсы
#add_custom_command(TARGET ${PROJECT_NAME}  POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_directory
#                ${CMAKE_CURRENT_SOURCE_DIR}/application/resources
#                ${PROJECT_BINARY_DIR}/$<CONFIG>/resources)

set(source_dir "${CMAKE_CURRENT_SOURCE_DIR}/application/resources")
set(dest_dir "${PROJECT_BINARY_DIR}/$<CONFIG>/resources")

add_custom_command(TARGET ${PROJECT_NAME}
	POST_BUILD
	COMMAND ${CMAKE_COMMAND}
    	-Dsrc_dir="${source_dir}/assets"
    	-Ddst_dir="${dest_dir}/assets"
    	-P "${CMAKE_CURRENT_SOURCE_DIR}/j4f/Tools/cmake/update_directory.cmake"
)

#if(j4f_PLATFORM STREQUAL "WINDOWS")
#	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
#endif()

if(j4f_PLATFORM STREQUAL "WINDOWS")
# конвертация шейдеров
	#add_custom_command(TARGET ${PROJECT_NAME}
    #               POST_BUILD
    #               COMMAND ${CMAKE_COMMAND} -E make_directory ${dest_dir}/shaders
    #               COMMAND cmd /c ${CMAKE_CURRENT_SOURCE_DIR}/j4f/Tools/glslToSpirv/converter_to_build_win.cmd ${dest_dir}/shaders ${source_dir}/shaders)


	add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E make_directory ${dest_dir}/shaders
                   COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/j4f/Tools/glslToSpirv/convert_with_includes_ext.py -i ${source_dir}/shaders/ -o ${dest_dir}/shaders/ -p /Bin/glslc.exe)
                   
elseif(j4f_PLATFORM STREQUAL "LINUX")
	add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E make_directory ${dest_dir}/shaders
                   COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/j4f/Tools/glslToSpirv/convert_with_includes_ext.py -i ${source_dir}/shaders/ -o ${dest_dir}/shaders/ -p /bin/glslc)
endif()

message(STATUS "!!!!!project generate complete!!!!!")