cmake_minimum_required(VERSION 3.20)
project(FashionStoreSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_executable(fashion_store
    src/main.cpp
)

target_include_directories(fashion_store PRIVATE src)

add_executable(fashion_store_core_smoke
    tests/core_smoke.cpp
)

target_include_directories(fashion_store_core_smoke PRIVATE src)

add_test(NAME fashion_store_core_smoke COMMAND fashion_store_core_smoke)
