#include <cassert>
#include <chrono>
#include <optional>

#include "application/cart/cart_application_service.hpp"
#include "application/catalog/catalog_application_service.hpp"
#include "application/order/order_application_service.hpp"
#include "application/staff/store_management_service.hpp"
#include "domain/pricing/voucher.hpp"
#include "infrastructure/persistence/in_memory/in_memory_repositories.hpp"

int main() {
    using namespace fashion_store::application::cart;
    using namespace fashion_store::application::catalog;
    using namespace fashion_store::application::order;
    using namespace fashion_store::application::staff;
    using namespace fashion_store::domain::catalog;
    using namespace fashion_store::domain::inventory;
    using namespace fashion_store::domain::order;
    using namespace fashion_store::domain::pricing;
    using namespace fashion_store::domain::shared;
    using namespace fashion_store::infrastructure::persistence::in_memory;

    InMemoryProductRepository product_repository;
    InMemoryInventoryRepository inventory_repository;
    InMemoryCartRepository cart_repository;
    InMemoryOrderRepository order_repository;
    InMemoryVoucherRepository voucher_repository;

    OrderApplicationService order_service(
        cart_repository,
        order_repository,
        product_repository,
        inventory_repository,
        voucher_repository);
    StoreManagementService staff_service(
        product_repository,
        inventory_repository,
        order_repository,
        voucher_repository,
        order_service);

    auto coat = staff_service.create_product(ProductDraft{
        ProductId{"product-coat"},
        "Structured Coat",
        Category::Outerwear,
        "Black wool coat",
        "Resort 2026",
        ProductStatus::Active});
    assert(coat);

    auto dress = staff_service.create_product(ProductDraft{
        ProductId{"product-dress"},
        "Column Dress",
        Category::Dresses,
        "Ivory evening dress",
        "Resort 2026",
        ProductStatus::Active});
    assert(dress);

    auto duplicate = staff_service.create_product(ProductDraft{
        ProductId{"product-coat"},
        "Replacement Coat",
        Category::Outerwear,
        "Should not overwrite existing product",
        "Resort 2026",
        ProductStatus::Active});
    assert(!duplicate);
    assert(duplicate.error() == StoreManagementError::ProductAlreadyExists);
    assert(product_repository.list_all().size() == 2);

    assert(staff_service.add_product_variant(ProductId{"product-coat"}, ProductVariantDraft{
        VariantId{"variant-coat-m"},
        "COAT-BLK-M",
        Size{"M"},
        Color{"Black"},
        Money::from_minor(4900000)}));
    auto updated_coat = staff_service.update_product(ProductDraft{
        ProductId{"product-coat"},
        "Tailored Coat",
        Category::Tops,
        "Refined black jacket",
        "PreFall 2026",
        ProductStatus::Draft});
    assert(updated_coat);
    assert(updated_coat.value().name() == "Tailored Coat");
    assert(updated_coat.value().category() == Category::Tops);
    assert(updated_coat.value().description() == "Refined black jacket");
    assert(updated_coat.value().collection() == "PreFall 2026");
    assert(updated_coat.value().status() == ProductStatus::Draft);
    assert(updated_coat.value().variants().size() == 1);
    assert(updated_coat.value().variants().front().id == VariantId{"variant-coat-m"});
    assert(staff_service.add_product_variant(ProductId{"product-dress"}, ProductVariantDraft{
        VariantId{"variant-dress-s"},
        "DRESS-IVY-S",
        Size{"S"},
        Color{"Ivory"},
        Money::from_minor(6500000)}));

    auto missing_update = staff_service.update_product(ProductDraft{
        ProductId{"missing-product"},
        "Missing",
        Category::Accessories,
        "Missing",
        "",
        ProductStatus::Active});
    assert(!missing_update);
    assert(missing_update.error() == StoreManagementError::ProductNotFound);

    auto inventory = staff_service.set_inventory(VariantId{"variant-coat-m"}, 3);
    assert(inventory);
    auto restocked = staff_service.restock_inventory(VariantId{"variant-coat-m"}, 2);
    assert(restocked);
    assert(restocked.value().on_hand() == 5);

    const auto now = std::chrono::system_clock::now();
    staff_service.save_voucher(Voucher(
        "STAFF10",
        DiscountType::Percentage,
        10,
        Money::from_minor(1000000),
        Money::from_minor(500000),
        now - std::chrono::hours(1),
        now + std::chrono::hours(24),
        5,
        true));
    assert(voucher_repository.find_by_code("STAFF10").has_value());

    CatalogApplicationService catalog_service(product_repository);
    auto black_outerwear = catalog_service.search_products(ProductSearchQuery{
        std::optional<std::string>{"coat"},
        std::optional<Category>{Category::Outerwear},
        std::optional<Size>{Size{"M"}},
        std::optional<Color>{Color{"Black"}},
        std::nullopt,
        std::optional<Money>{Money::from_minor(5000000)},
        std::optional<std::string>{"Resort 2026"},
        std::optional<ProductStatus>{ProductStatus::Active},
        ProductSortMode::PriceAsc});
    assert(black_outerwear.empty());

    auto tailored_tops = catalog_service.search_products(ProductSearchQuery{
        std::optional<std::string>{"tailored"},
        std::optional<Category>{Category::Tops},
        std::optional<Size>{Size{"M"}},
        std::optional<Color>{Color{"Black"}},
        std::nullopt,
        std::optional<Money>{Money::from_minor(5000000)},
        std::optional<std::string>{"PreFall 2026"},
        std::optional<ProductStatus>{ProductStatus::Draft},
        ProductSortMode::PriceAsc});
    assert(tailored_tops.size() == 1);
    assert(tailored_tops.front().id() == ProductId{"product-coat"});

    auto sorted = catalog_service.search_products(ProductSearchQuery{
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::optional<std::string>{"Resort 2026"},
        std::optional<ProductStatus>{ProductStatus::Active},
        ProductSortMode::PriceDesc});
    assert(sorted.size() == 1);
    assert(sorted.front().id() == ProductId{"product-dress"});

    return 0;
}
