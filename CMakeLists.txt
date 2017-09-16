cmake_minimum_required(VERSION 2.8)

project(simulator)

configure_file (
  "SimulatorConfig.h.in"
  "SimulatorConfig.h"
  )

option (USE_LOG 
        "enable LOG" OFF) 
set(sim_srcs
  alloc.cc
  buffer.cc
  FS.cc
  compressor.cc
  c_snappy.cc
  main.cc)

set (EXTRA_LIBS ${EXTRA_LIBS} snappy)

add_executable(simulator ${sim_srcs})

target_link_libraries(simulator ${EXTRA_LIBS})
