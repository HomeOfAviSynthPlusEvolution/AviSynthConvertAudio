include(FetchContent)
if(NOT TARGET GTest::gtest_main)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
  if(MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  endif()
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(googletest)
endif()
FetchContent_Declare(xxhash
  GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
  GIT_TAG v0.8.3 GIT_SHALLOW TRUE
  SOURCE_SUBDIR cmake_unneeded)
FetchContent_MakeAvailable(xxhash)

add_executable(convert_audio_tests convert_audio_tests.cpp api_tests.cpp)
target_compile_features(convert_audio_tests PRIVATE cxx_std_17)
target_include_directories(convert_audio_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src ${xxhash_SOURCE_DIR})
target_link_libraries(convert_audio_tests PRIVATE AudioConvert GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(convert_audio_tests TEST_PREFIX "convert_audio.")

# Compile and call the public interface from an actual C translation unit.
add_executable(c_api_smoke c_api_smoke.c)
set_target_properties(c_api_smoke PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(c_api_smoke PRIVATE AudioConvert)
add_test(NAME audio_convert.c_api COMMAND c_api_smoke)
