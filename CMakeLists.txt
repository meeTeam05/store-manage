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
    src/infrastructure/persistence/file/file_store.cpp
    src/infrastructure/persistence/file/file_repositories.cpp
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

add_executable(fashion_store_file_persistence_smoke
    tests/file_persistence_smoke.cpp
)

target_link_libraries(fashion_store_file_persistence_smoke PRIVATE fashion_store_core)

add_test(NAME fashion_store_file_persistence_smoke COMMAND fashion_store_file_persistence_smoke)

add_executable(fashion_store_order_consistency_smoke
    tests/order_consistency_smoke.cpp
)

target_link_libraries(fashion_store_order_consistency_smoke PRIVATE fashion_store_core)

add_test(NAME fashion_store_order_consistency_smoke COMMAND fashion_store_order_consistency_smoke)

add_executable(fashion_store_review_return_guards_smoke
    tests/review_return_guards_smoke.cpp
)

target_link_libraries(fashion_store_review_return_guards_smoke PRIVATE fashion_store_core)

add_test(NAME fashion_store_review_return_guards_smoke COMMAND fashion_store_review_return_guards_smoke)

add_executable(fashion_store_return_management_smoke
    tests/return_management_smoke.cpp
)

target_link_libraries(fashion_store_return_management_smoke PRIVATE fashion_store_core)

add_test(NAME fashion_store_return_management_smoke COMMAND fashion_store_return_management_smoke)

add_executable(fashion_store_report_smoke
    tests/report_smoke.cpp
)

target_link_libraries(fashion_store_report_smoke PRIVATE fashion_store_core)

add_test(NAME fashion_store_report_smoke COMMAND fashion_store_report_smoke)
