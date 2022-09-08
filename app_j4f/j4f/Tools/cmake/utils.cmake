#cmake/utils.cmake

macro(add_3rdparty_project SOURCE_DIR BINARY_DIR)
  set (SAVED_C_FLAGS ${CMAKE_C_FLAGS})
  set (SAVED_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
  set (SAVED_CXX_FLAGS ${CMAKE_CXX_FLAGS})  
  set (SAVED_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})

  if (MSVC)
    # Microsoft
    set (CMAKE_C_FLAGS "${THIRD_C_FLAGS} /w")
    set (CMAKE_CXX_FLAGS "${THIRD_CXX_FLAGS} /w")
  else()
    # Other compiler
    set (CMAKE_C_FLAGS "${THIRD_C_FLAGS} -w -Wno-everything")
    set (CMAKE_CXX_FLAGS "${THIRD_CXX_FLAGS} -w -Wno-everything")
  endif()

  add_subdirectory(${SOURCE_DIR} ${BINARY_DIR})

  set (CMAKE_C_FLAGS ${SAVED_C_FLAGS})
  set (CMAKE_C_FLAGS_DEBUG ${SAVED_C_FLAGS_DEBUG})
  set (CMAKE_CXX_FLAGS ${SAVED_CXX_FLAGS})
  set (CMAKE_CXX_FLAGS_DEBUG ${SAVED_CXX_FLAGS_DEBUG})
endmacro(add_3rdparty_project)