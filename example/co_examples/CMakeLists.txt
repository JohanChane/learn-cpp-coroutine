cmake_minimum_required(VERSION 3.15)

project(app VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER "g++")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

#set(CMAKE_BUILD_TYPE "Release")
#set(CMAKE_BUILD_TYPE "Debug")

#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#file(GLOB hdrs "src/*.h")
file(GLOB srcs "src/*.cc")
set(target ${PROJECT_NAME})
add_executable(${target}
  ${srcs}
)

#set(target ${PROJECT_NAME})
#add_library(${target} SHARED
#  ${srcs}
#)

#target_include_directories(${target}
#  PUBLIC
#  <include_dirs>
#)

#target_link_libraries(${target}
#  <static/shared libraries>
#)

#find_package(<package_name> REQUIRED)
#target_link_libraries(${target}
#  <package_name>::<target_name>
#)

add_custom_target(run_${target} ALL 
  COMMAND ${CMAKE_COMMAND} -E echo "Executing ${target}..."
  COMMAND $<TARGET_FILE:${target}>
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  DEPENDS ${target}
)
