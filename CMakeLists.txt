# Project Chimera VPU Build System
cmake_minimum_required(VERSION 3.15)
project(ChimeraVPU CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Define the VPU Core Library ---
# This library contains the main logic of all five pillars.
add_library(vpu_core
    src/vpu_core.cpp
    src/hal/cpu_kernels.cpp
    src/core/Pillar2_Cortex.cpp
    src/core/Pillar3_Orchestrator.cpp
    src/core/Pillar4_Cerebellum.cpp
    src/core/Pillar5_Feedback.cpp
)

# Public headers for the library are in the 'api' directory.
target_include_directories(vpu_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Placeholder for finding an external FFTW library
# In a real project, you would uncomment this and link properly.
find_package(FFTW3 REQUIRED)
target_link_libraries(vpu_core PRIVATE FFTW3::fftw3)

# --- Define Test Executables ---
add_executable(e2e_full_loop tests/e2e_full_loop.cpp)

# Link the test against our VPU library so it can use its functionality
target_link_libraries(e2e_full_loop PRIVATE vpu_core)

# --- Enable Testing with CTest ---
enable_testing()
add_test(NAME EndToEndLearningTest COMMAND e2e_full_loop)
