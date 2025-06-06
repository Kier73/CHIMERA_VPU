
import { ProjectDataType, FileNode } from '../types';

const repoStructureData: FileNode[] = [
  {
    name: 'CHIMERA_VPU/',
    type: 'directory',
    children: [
      {
        name: 'api/',
        type: 'directory',
        children: [
          { name: 'vpu.h', type: 'file', comment: 'Public-facing API for developers to include.' }
        ]
      },
      {
        name: 'src/',
        type: 'directory',
        children: [
          {
            name: 'core/',
            type: 'directory',
            children: [
              { name: 'Pillar1_Synapse.h/cpp', type: 'file', comment: 'Task interception & API implementation.' },
              { name: 'Pillar2_Cortex.h/cpp', type: 'file', comment: 'Flux Profiler & Hardware Database.' },
              { name: 'Pillar3_Orchestrator.h/cpp', type: 'file', comment: 'Decision engine & path simulation.' },
              { name: 'Pillar4_Cerebellum.h/cpp', type: 'file', comment: 'Dispatcher & JIT/runtime management.' },
              { name: 'Pillar5_Feedback.h/cpp', type: 'file', comment: 'Learning loop and belief updates.' },
            ]
          },
          {
            name: 'hal/',
            type: 'directory',
            children: [
              { name: 'hal.h', type: 'file', comment: 'Common interface for all hardware kernels.' },
              { name: 'cpu_kernels.cpp', type: 'file', comment: 'Optimized SIMD/AVX kernels.' },
              { name: 'gpu_kernels.cu', type: 'file', comment: 'GPU-specific CUDA/ROCm kernels.' },
            ]
          },
          { name: 'vpu_core.cpp', type: 'file', comment: 'Main VPU class that orchestrates the pillars.' }
        ]
      },
      {
        name: 'tests/',
        type: 'directory',
        children: [
          { name: 'e2e_first_loop.cpp', type: 'file', comment: 'First end-to-end integration test.'}
        ]
      },
      {
        name: 'external/',
        type: 'directory',
        children: [
          { name: 'fftw/', type: 'file', comment: 'Placeholder for third-party libs like FFTW.' }
        ]
      },
      { name: 'CMakeLists.txt', type: 'file', comment: 'The master build file for the project.' }
    ]
  }
];

const cmakeFileContentData = `
# Project Chimera VPU Build System
cmake_minimum_required(VERSION 3.15)

project(ChimeraVPU CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Find External Dependencies ---
# Placeholder for finding the FFTW library
# find_package(FFTW3 REQUIRED)

# --- Define the VPU Core Library ---
# This library contains the main logic of all five pillars.
add_library(vpu_core
  src/core/Pillar1_Synapse.cpp
  src/core/Pillar2_Cortex.cpp
  src/core/Pillar3_Orchestrator.cpp
  src/core/Pillar4_Cerebellum.cpp
  src/core/Pillar5_Feedback.cpp
  src/hal/cpu_kernels.cpp
  src/vpu_core.cpp
)

# Public headers for the library
target_include_directories(vpu_core PUBLIC
  \\${CMAKE_CURRENT_SOURCE_DIR}/api