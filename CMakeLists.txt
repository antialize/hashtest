cmake_minimum_required(VERSION 3.10)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(hash test.cpp
  crunch/crnlib/crn_core.cpp
  crunch/crnlib/crn_mem.cpp
  crunch/crnlib/crn_assert.cpp
  crunch/crnlib/crn_platform.cpp
  crunch/crnlib/crn_vector.cpp
  tpie/tpie/memory.cpp
  tpie/tpie/prime.cpp
  tpie/tpie/hash.cpp
  tp2/tpie_log.cpp
  tp2/tpie.cpp
  tpie/tpie/resource_manager.cpp
  tpie/tpie/logstream.cpp
  tpie/tpie/resources.cpp
  ) 
target_link_libraries(hash Threads::Threads)
target_include_directories(hash PRIVATE flat_hash_map crunch/crnlib/ crunch/inc)
target_include_directories(hash PRIVATE flat_hash_map crunch/crnlib/ tpie tp2)

set_target_properties(hash PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)
