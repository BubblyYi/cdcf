cmake_minimum_required(VERSION 3.10)
project(cdcf)

set(CMAKE_CXX_STANDARD 17)
if("${CONAN_COMPILER}" STREQUAL "gcc")
    set(CMAKE_C_FLAGS ${CONAN_C_FLAGS})
    set(CMAKE_CXX_FLAGS ${CONAN_CXX_FLAGS})
    set(CMAKE_SHARED_LINKER_FLAGS "${CONAN_SHARED_LINKER_FLAGS}")
endif()

include(CTest)
add_subdirectory(node_keeper)
add_subdirectory(actor_system)
add_subdirectory(demos)
add_subdirectory(config_manager)