##

if (NOT(EXE))
  set(EXE main)
endif()

project(${EXE})

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  set(FLAG_DEBUG)
else()
  set(FLAG_DEBUG -D_DEBUG_)
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
find_package(InferenceEngine 2022 REQUIRED)
find_package(OpenCV 4.5 REQUIRED)

set(IE_ROOT_DIR $ENV{INTEL_OPENVINO_DIR})
set(OMZ_ROOT_DIR $ENV{HOME}/github/open_model_zoo-2022.2.0/demos)

include_directories(
  ${IE_ROOT_DIR}/runtime/include
  ${IE_ROOT_DIR}/runtime/include/ie
  
  ${OMZ_ROOT_DIR}/common/cpp/models/include
  # ${OMZ_ROOT_DIR}/common/cpp/monitors/include
  # ${OMZ_ROOT_DIR}/common/cpp/pipelines/include
  ${OMZ_ROOT_DIR}/common/cpp/utils/include
  )

file (GLOB_RECURSE LIBRARY_FILES
  ${OMZ_ROOT_DIR}/common/cpp/models/src/hpe_model_openpose.cpp
  ${OMZ_ROOT_DIR}/common/cpp/models/src/openpose_decoder.cpp
  ${OMZ_ROOT_DIR}/common/cpp/models/src/image_model.cpp
  ${OMZ_ROOT_DIR}/common/cpp/models/src/model_base.cpp

  ${OMZ_ROOT_DIR}/common/cpp/utils/src/args_helper.cpp
  ${OMZ_ROOT_DIR}/common/cpp/utils/src/image_utils.cpp
  )

# #
# # TBB
# #
# if (EXISTS $ENV{TBB_DIR})
#   include_directories($ENV{TBB_DIR}/../include)
#   link_directories($ENV{TBB_DIR}/../lib)
# endif()

#
# lib
#
string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} ARCH)

if (ARCH STREQUAL x86_64)
  link_directories(${IE_ROOT_DIR}/runtime/lib/intel64/Release)
else()
  link_directories(${IE_ROOT_DIR}/runtime/lib/${ARCH}/Release)
endif()

#
# M1(arm64:homebrew) or not
#
if (ARCH STREQUAL "arm64")
  include_directories(/opt/homebrew/include)
  link_directories(/opt/homebrew/lib)
  set(CMAKE_CXX_FLAGS "-std=c++11 -O3 -Wno-deprecated-declarations")
else()
  include_directories(/usr/local/include)
  link_directories(/usr/local/lib)
  set(CMAKE_CXX_FLAGS "-std=c++11 -O3 -march=native -Wno-deprecated ")
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

file (GLOB_RECURSE SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/image-server.cpp
  )

add_definitions(-DTBB_SUPPRESS_DEPRECATED_MESSAGES ${FLAG_DEBUG} -DHOME_DIR="$ENV{HOME}")
add_executable (${EXE} ${SOURCE_FILES} ${LIBRARY_FILES})

target_link_libraries (${EXE} PRIVATE
  ${OpenCV_LIBS}
  openvino
  tbb gflags zmq
)


# end
