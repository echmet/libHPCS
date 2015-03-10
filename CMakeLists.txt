cmake_minimum_required(VERSION 2.6)
include(TestBigEndian)

project(libHPCS)

add_definitions("-std=c89 -Wall")

test_big_endian(HAVE_BIG_ENDIAN)
if (${HAVE_BIG_ENDIAN})
  add_definitions(-D_HPCS_BIG_ENDIAN)
else()
  add_definitions(-D_HPCS_LITTLE_ENDIAN)
endif()

set(libHPCS_SRCS
    libhpcs.c)

add_library(HPCS SHARED ${libHPCS_SRCS})
set_target_properties(HPCS
		      PROPERTIES VERSION 1.0
		      SOVERSION 1.0)

if (${BuildTestTool})
  set(libHPCS_test_SRCS
      test_tool.c)

  add_executable(test_tool ${libHPCS_test_SRCS})
  target_link_libraries(test_tool HPCS)
endif()
