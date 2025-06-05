cmake_minimum_required(VERSION 3.10)
project(Chimera_VPU CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF) # Avoid compiler-specific extensions

# --- Library: vpu_core ---
# Gather all source files for the vpu_core library
# This makes it easier to manage as more files are added to pillars or core
set(VPU_CORE_SOURCES
    src/core/VPUCore.cpp
    api/vpu.cpp # Added vpu.cpp for the VPU class implementation
    src/pillars/Cortex.cpp
    src/pillars/Orchestrator.cpp
    src/pillars/Cerebellum.cpp
    src/pillars/Feedback.cpp
    src/hal/CPUDenseKernels.cpp
)

add_library(vpu_core STATIC
    ${VPU_CORE_SOURCES}
)

# Target include directories for vpu_core
# PUBLIC: Anything linking to vpu_core also needs these include paths
# INTERFACE: Anything linking to vpu_core needs these, but vpu_core itself doesn't when compiling
# PRIVATE: Only vpu_core needs these when compiling
target_include_directories(vpu_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/api # For vpu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src  # For VPUCore.h, pillar headers, hal headers
)
target_include_directories(vpu_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pillars
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal
)


# --- Executable: chimera_test ---
add_executable(chimera_test
    tests/main.cpp
)

# Link chimera_test against vpu_core
target_link_libraries(chimera_test PRIVATE vpu_core)

# Set properties for Visual Studio if applicable (useful for debugging)
# set_property(TARGET chimera_test PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# --- Testing (Optional but Recommended) ---
# enable_testing()
# add_test(NAME ChimeraBasicRun COMMAND chimera_test)
# You might need to specify the configuration for multi-config generators if running tests via CTest
# set_tests_properties(ChimeraBasicRun PROPERTIES CONFIGURATIONS "Debug;Release")


# --- Output Messages ---
message(STATUS "------------------------------------------------------------------")
message(STATUS "Configured Chimera_VPU project (C++ Standard: ${CMAKE_CXX_STANDARD})")
message(STATUS "Static Library 'vpu_core' will be built from:")
foreach(SOURCE_FILE ${VPU_CORE_SOURCES})
    message(STATUS "  - ${SOURCE_FILE}")
endforeach()
message(STATUS "Executable 'chimera_test' will be built from tests/main.cpp")
message(STATUS "Linking 'chimera_test' against 'vpu_core'")
message(STATUS "Include paths for 'vpu_core' (Public):")
message(STATUS "  - ${CMAKE_CURRENT_SOURCE_DIR}/api")
message(STATUS "  - ${CMAKE_CURRENT_SOURCE_DIR}/src")
message(STATUS "------------------------------------------------------------------")
