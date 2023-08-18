cmake_minimum_required (VERSION 3.11)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/ftxui)

project(cachetuna
   LANGUAGES CXX
   VERSION 1.0.0
)

add_executable(cachetuna 
   src/main.cpp
   src/pqos_util.cpp
   src/ui.cpp
   src/graph.cpp
   src/misc.cpp
   src/autotuna.cpp
   src/braille_generator.cpp
   )

target_include_directories(cachetuna PRIVATE include)
target_include_directories(cachetuna PRIVATE third_party/pqos/include)
target_include_directories(cachetuna PRIVATE third_party/stb_img/include)

target_link_directories(cachetuna PRIVATE third_party/pqos/lib)

target_link_libraries(cachetuna
   PRIVATE pqos
   PRIVATE screen
   PRIVATE dom
   PRIVATE component
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/img DESTINATION ${CMAKE_BINARY_DIR})
