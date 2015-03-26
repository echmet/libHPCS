cmake_minimum_required(VERSION 2.6)
include(TestBigEndian)

project(libHPCS)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
add_definitions("-std=c89 -Wall -Wextra")

if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
  add_definitions("-ggdb -g3")
endif()

test_big_endian(HAVE_BIG_ENDIAN)
if (${HAVE_BIG_ENDIAN})
  add_definitions(-D_HPCS_BIG_ENDIAN)
else()
  add_definitions(-D_HPCS_LITTLE_ENDIAN)
endif()

find_package(ICU 54 REQUIRED COMPONENTS uc io)

set(libHPCS_SRCS
    libhpcs.c)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${ICU_INCLUDE_DIRS})

add_library(HPCS SHARED ${libHPCS_SRCS})
target_link_libraries(HPCS PRIVATE ${ICU_LIBRARIES})
set_target_properties(HPCS
		      PROPERTIES VERSION 2.0
		      SOVERSION 2.0)

if (${BuildTestTool})
  set(libHPCS_test_SRCS
      test_tool.c)

  add_executable(test_tool ${libHPCS_test_SRCS})
  target_link_libraries(test_tool HPCS)
endif()
