cmake_minimum_required(VERSION 3.24)
project(AviSynthConvertAudio VERSION 0.1.0 LANGUAGES C CXX)

include(TestBigEndian)
test_big_endian(AC_BIG_ENDIAN)
if(AC_BIG_ENDIAN)
  message(FATAL_ERROR "The imported PCM kernels currently require little-endian storage.")
endif()

option(AC_BUILD_TESTS "Build independent conversion tests" ${PROJECT_IS_TOP_LEVEL})
option(AC_BUILD_BENCHMARKS "Build the native/C conversion benchmark" OFF)
option(AC_SCALAR_ONLY "Build ordinary C fallback without SIMD targets" OFF)

if(NOT TARGET hwy)
  set(HWY_ENABLE_CONTRIB OFF CACHE BOOL "" FORCE)
  set(HWY_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(HWY_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
  set(HWY_ENABLE_TOOLS OFF CACHE BOOL "" FORCE)
  set(HWY_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
  set(HWY_LIBRARY_TYPE STATIC CACHE STRING "" FORCE)
  set(HWY_FORCE_STATIC_LIBS ON CACHE BOOL "" FORCE)
  set(HWY_CMAKE_RVV OFF CACHE BOOL "" FORCE)
  add_subdirectory(third_party/highway EXCLUDE_FROM_ALL)
endif()

add_library(AudioConvert STATIC
  src/api.cpp src/kernels_scalar.cpp src/kernels_composed.cpp
  src/kernels_highway.cpp)
add_library(AviSynth::ConvertAudio ALIAS AudioConvert)
target_compile_features(AudioConvert PRIVATE cxx_std_17)
set_target_properties(AudioConvert PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(AudioConvert PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(AudioConvert PRIVATE hwy)
if(WIN32)
  target_compile_definitions(AudioConvert PRIVATE NOMINMAX)
endif()
if(AC_SCALAR_ONLY)
  target_compile_definitions(AudioConvert PRIVATE HWY_COMPILE_ONLY_SCALAR)
endif()

if(AC_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
if(AC_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
