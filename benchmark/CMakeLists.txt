cmake_minimum_required(VERSION 3.15)
project(HapiBenchmark CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(../include)

# Se não passarmos nada, o padrão é 50
if(NOT DEFINED TEST_SIZE)
    set(TEST_SIZE 50)
endif()

# Passa o tamanho para o código fonte como macro de pré-processador
add_compile_definitions(TEST_SIZE=${TEST_SIZE})

add_executable(bench_baseline main.cpp)
target_compile_definitions(bench_baseline PRIVATE TEST_BASELINE)

add_executable(bench_tuple main.cpp)
target_compile_definitions(bench_tuple PRIVATE TEST_TUPLE)

add_executable(bench_hapi main.cpp)
target_compile_definitions(bench_hapi PRIVATE TEST_HAPI)