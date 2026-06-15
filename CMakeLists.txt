cmake_minimum_required(VERSION 3.20)
project(FashionStoreSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_library(fashion_store_core
    src/domain/catalog/product.cpp
    src/domain/inventory/inventory_item.cpp
    src/domain/cart/cart.cpp
    src/domain/order/order.cpp
    src/domain/pricing/voucher.cpp
    src/domain/customer/customer.cpp
    src/domain/identity/account.cpp
)

target_include_directories(fashion_store_core PUBLIC src)

add_executable(fashion_store
    src/main.cpp
)

target_link_libraries(fashion_store PRIVATE fashion_store_core)

add_executable(fashion_store_core_smoke
    tests/core_smoke.cpp
)

target_link_libraries(fashion_store_core_smoke PRIVATE fashion_store_core)

add_test(NAME fashion_store_core_smoke COMMAND fashion_store_core_smoke)
