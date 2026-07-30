##
#  CMake script for the WorkStream benchmark.
##

set(TARGET "workstream-benchmark")

set(TARGET_SRC
  ${TARGET}.cc
  )

cmake_minimum_required(VERSION 3.13.4)

find_package(deal.II 9.6.0
  HINTS ${deal.II_DIR} ${DEAL_II_DIR} ../ ../../ $ENV{DEAL_II_DIR}
  )
if(NOT ${deal.II_FOUND})
  message(FATAL_ERROR "\n"
    "*** Could not locate a (sufficiently recent) version of deal.II. ***\n\n"
    "You may want to either pass a flag -DDEAL_II_DIR=/path/to/deal.II to cmake\n"
    "or set an environment variable \"DEAL_II_DIR\" that contains this path."
    )
endif()


deal_ii_initialize_cached_variables()
project(${TARGET})

if(NOT CMAKE_BUILD_TYPE MATCHES "Release")
  message(WARNING "\n"
    "*** CMAKE_BUILD_TYPE is \"${CMAKE_BUILD_TYPE}\". ***\n"
    )
endif()

deal_ii_invoke_autopilot()
