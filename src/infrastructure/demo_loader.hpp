#pragma once

#include <filesystem>
#include "infrastructure/persistence/file/file_repositories.hpp"

namespace fashion_store::application::cart {
class CartApplicationService;
}
namespace fashion_store::application::order {
class OrderApplicationService;
}

namespace fashion_store::infrastructure {

void initialize_demo_state(
    fashion_store::infrastructure::persistence::file::FileProductRepository& product_repository,
    fashion_store::infrastructure::persistence::file::FileInventoryRepository& inventory_repository,
    fashion_store::infrastructure::persistence::file::FileCustomerRepository& customer_repository,
    fashion_store::infrastructure::persistence::file::FileAccountRepository& account_repository,
    fashion_store::infrastructure::persistence::file::FileVoucherRepository& voucher_repository,
    fashion_store::infrastructure::persistence::file::FileOrderRepository& order_repository,
    fashion_store::application::cart::CartApplicationService& cart_svc,
    fashion_store::application::order::OrderApplicationService& order_svc,
    const std::filesystem::path& demo_dir,
    const std::filesystem::path& product_storefront_path
);

} // namespace fashion_store::infrastructure
