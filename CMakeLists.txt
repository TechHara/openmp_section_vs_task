cmake_minimum_required(VERSION 3.0.0)
project(omp_section_vs_task VERSION 0.1.0)

find_package(OpenMP REQUIRED)
set(CMAKE_CXX_STANDARD 17)

add_executable(quicksort main.cc)
target_link_libraries(quicksort PRIVATE OpenMP::OpenMP_CXX)
