cmake_minimum_required(VERSION 2.6)
include(TestBigEndian)

project(libHPCS)

option(BUILD_TEST_TOOL "Build a simple test tool to check the library's operation" OFF)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
if (NOT MSVC)
    add_definitions("-std=c89 -Wall -Wextra -fvisibility=hidden")
	if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
        add_definitions("-ggdb -g3")
    endif()
endif()


if (WIN32)
    set(WIN32_EXTRA_LIBS Shlwapi)
else()
    set(WIN32_EXTRA_LIBS "")
endif()

test_big_endian(HAVE_BIG_ENDIAN)
if (${HAVE_BIG_ENDIAN})
  add_definitions(-D_HPCS_BIG_ENDIAN)
else()
  add_definitions(-D_HPCS_LITTLE_ENDIAN)
endif()

if (NOT WIN32)
    find_package(ICU 52 REQUIRED COMPONENTS uc io)
else()
    set(ICU_INCLUDE_DIRS "")
endif()

set(libHPCS_SRCS
    src/libHPCS.c)

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  ${ICU_INCLUDE_DIRS})

add_library(HPCS SHARED ${libHPCS_SRCS})
target_link_libraries(HPCS PRIVATE ${ICU_LIBRARIES} ${WIN32_EXTRA_LIBS})
set_target_properties(HPCS
                      PROPERTIES VERSION 5.0
                                 SOVERSION 5.0
                                 PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/include/libHPCS.h")
if (MSVC)
    set_target_properties(HPCS
                          PROPERTIES PREFIX "lib")
endif()

if (NOT WIN32)
    install(TARGETS HPCS
            LIBRARY DESTINATION lib
            PUBLIC_HEADER DESTINATION include)
else()
    install(TARGETS HPCS
            RUNTIME DESTINATION lib
            PUBLIC_HEADER DESTINATION include)
endif()


if (BUILD_TEST_TOOL)
  set(libHPCS_test_SRCS
      test_tool.c)

  add_executable(test_tool ${libHPCS_test_SRCS})
  target_link_libraries(test_tool HPCS)
endif()
