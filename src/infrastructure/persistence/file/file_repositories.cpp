#include "infrastructure/persistence/file/file_repositories.hpp"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "infrastructure/persistence/file/file_store.hpp"

namespace fashion_store::infrastructure::persistence::file {

namespace {

using fashion_store::domain::cart::Cart;
using fashion_store::domain::cart::CartItem;
using fashion_store::domain::catalog::CatalogVariantView;
using fashion_store::domain::catalog::Category;
using fashion_store::domain::catalog::Product;
using fashion_store::domain::catalog::ProductStatus;
using fashion_store::domain::catalog::ProductVariant;
using fashion_store::domain::customer::Customer;
using fashion_store::domain::identity::Account;
using fashion_store::domain::identity::AccountStatus;
using fashion_store::domain::identity::Role;
using fashion_store::domain::inventory::InventoryItem;
using fashion_store::domain::order::Order;
using fashion_store::domain::order::OrderItem;
using fashion_store::domain::order::OrderStatus;
using fashion_store::domain::order::PaymentMethod;
using fashion_store::domain::order::PaymentStatus;
using fashion_store::domain::pricing::DiscountType;
using fashion_store::domain::pricing::Voucher;
using fashion_store::domain::review::Review;
using fashion_store::domain::returns::ReturnRequest;
using fashion_store::domain::returns::ReturnStatus;
using fashion_store::domain::shared::AccountId;
using fashion_store::domain::shared::CartId;
using fashion_store::domain::shared::Color;
using fashion_store::domain::shared::CustomerId;
using fashion_store::domain::shared::Money;
using fashion_store::domain::shared::OrderId;
using fashion_store::domain::shared::OrderItemId;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::ReviewId;
using fashion_store::domain::shared::ReturnId;
using fashion_store::domain::shared::ShippingAddress;
using fashion_store::domain::shared::Size;
using fashion_store::domain::shared::VariantId;

template <typename T>
std::string encode_quoted(const T& value) {
    std::ostringstream output;
    output << std::quoted(value);
    return output.str();
}

std::chrono::system_clock::time_point from_epoch(std::int64_t epoch_seconds) {
    return std::chrono::system_clock::time_point{std::chrono::seconds{epoch_seconds}};
}

std::int64_t to_epoch(std::chrono::system_clock::time_point time_point) {
    return std::chrono::duration_cast<std::chrono::seconds>(time_point.time_since_epoch()).count();
}

template <typename Enum>
Enum enum_from_int(int value) {
    return static_cast<Enum>(value);
}

std::string serialize_address(const ShippingAddress& address) {
    std::ostringstream output;
    output << std::quoted(address.recipient_name) << '\t'
           << std::quoted(address.phone) << '\t'
           << std::quoted(address.line1) << '\t'
           << std::quoted(address.line2) << '\t'
           << std::quoted(address.ward) << '\t'
           << std::quoted(address.district) << '\t'
           << std::quoted(address.city) << '\t'
           << std::quoted(address.country);
    return output.str();
}

ShippingAddress deserialize_address(std::istringstream& input) {
    ShippingAddress address;
    input >> std::quoted(address.recipient_name)
          >> std::quoted(address.phone)
          >> std::quoted(address.line1)
          >> std::quoted(address.line2)
          >> std::quoted(address.ward)
          >> std::quoted(address.district)
          >> std::quoted(address.city)
          >> std::quoted(address.country);
    return address;
}

std::vector<Product> load_products(const std::filesystem::path& path) {
    struct ProductRecord {
        std::optional<Product> product;
        std::vector<ProductVariant> variants;
    };

    std::map<std::string, ProductRecord> records;
    for (const auto& line : read_all_lines(path)) {
        std::istringstream input(line);
        std::string tag;
        input >> tag;
        if (tag == "P") {
            std::string id;
            std::string name;
            std::string description;
            std::string collection;
            int category = 0;
            int status = 0;
            input >> std::quoted(id) >> std::quoted(name) >> category
                  >> std::quoted(description) >> std::quoted(collection) >> status;
            auto product = Product::rehydrate(
                ProductId{id},
                name,
                enum_from_int<Category>(category),
                description,
                collection,
                enum_from_int<ProductStatus>(status),
                {});
            records.emplace(id, ProductRecord{std::move(product), {}});
        } else if (tag == "V") {
            std::string product_id;
            std::string variant_id;
            std::string sku;
            std::string size;
            std::string color;
            std::int64_t price_minor = 0;
            bool active = true;
            input >> std::quoted(product_id) >> std::quoted(variant_id) >> std::quoted(sku)
                  >> std::quoted(size) >> std::quoted(color) >> price_minor >> active;
            auto record = records.find(product_id);
            if (record == records.end()) {
                throw std::runtime_error("variant encountered before product definition");
            }
            record->second.variants.push_back(ProductVariant{
                VariantId{variant_id},
                sku,
                Size{size},
                Color{color},
                Money::from_minor(price_minor),
                active
            });
        }
    }

    std::vector<Product> products;
    for (auto& [_, record] : records) {
        if (!record.product.has_value()) {
            throw std::runtime_error("product record missing root definition");
        }
        products.push_back(Product::rehydrate(
            record.product->id(),
            record.product->name(),
            record.product->category(),
            record.product->description(),
            record.product->collection(),
            record.product->status(),
            std::move(record.variants)));
    }
    return products;
}

void save_products(const std::filesystem::path& path, const std::vector<Product>& products) {
    std::vector<std::string> lines;
    for (const auto& product : products) {
        std::ostringstream product_line;
        product_line << "P "
                     << std::quoted(product.id().value) << '\t'
                     << std::quoted(product.name()) << '\t'
                     << static_cast<int>(product.category()) << '\t'
                     << std::quoted(product.description()) << '\t'
                     << std::quoted(product.collection()) << '\t'
                     << static_cast<int>(product.status());
        lines.push_back(product_line.str());

        for (const auto& variant : product.variants()) {
            std::ostringstream variant_line;
            variant_line << "V "
                         << std::quoted(product.id().value) << '\t'
                         << std::quoted(variant.id.value) << '\t'
                         << std::quoted(variant.sku) << '\t'
                         << std::quoted(variant.size.value) << '\t'
                         << std::quoted(variant.color.value) << '\t'
                         << variant.price.minor_units() << '\t'
                         << variant.active;
            lines.push_back(variant_line.str());
        }
    }
    write_all_lines(path, lines);
}

std::vector<InventoryItem> load_inventory_items(const std::filesystem::path& path) {
    std::vector<InventoryItem> items;
    for (const auto& line : read_all_lines(path)) {
        std::istringstream input(line);
        std::string tag;
        std::string variant_id;
        int on_hand = 0;
        int reserved = 0;
        input >> tag >> std::quoted(variant_id) >> on_hand >> reserved;
        if (tag == "I") {
            items.emplace_back(VariantId{variant_id}, on_hand, reserved);
        }
    }
    return items;
}

void save_inventory_items(const std::filesystem::path& path, const std::vector<InventoryItem>& items) {
    std::vector<std::string> lines;
    for (const auto& item : items) {
        std::ostringstream output;
        output << "I " << std::quoted(item.variant_id().value) << '\t'
               << item.on_hand() << '\t' << item.reserved();
        lines.push_back(output.str());
    }
    write_all_lines(path, lines);
}

std::vector<Cart> load_carts(const std::filesystem::path& path) {
    struct CartRecord {
        CustomerId customer_id;
        std::vector<CartItem> items;
    };

    std::map<std::string, CartRecord> records;
    for (const auto& line : read_all_lines(path)) {
        std::istringstream input(line);
        std::string tag;
        input >> tag;
        if (tag == "C") {
            std::string cart_id;
            std::string customer_id;
            input >> std::quoted(cart_id) >> std::quoted(customer_id);
            records.emplace(cart_id, CartRecord{CustomerId{customer_id}, {}});
        } else if (tag == "I") {
            std::string cart_id;
            std::string variant_id;
            int quantity = 0;
            std::int64_t price_minor = 0;
            input >> std::quoted(cart_id) >> std::quoted(variant_id) >> quantity >> price_minor;
            records[cart_id].items.push_back(CartItem{
                VariantId{variant_id},
                quantity,
                Money::from_minor(price_minor)
            });
        }
    }

    std::vector<Cart> carts;
    for (auto& [cart_id, record] : records) {
        Cart cart(CartId{cart_id}, record.customer_id);
        for (const auto& item : record.items) {
            auto result = cart.add_item(item.variant_id, item.quantity, item.unit_price_snapshot);
            if (!result) {
                throw std::runtime_error("failed to rehydrate cart");
            }
        }
        carts.push_back(cart);
    }
    return carts;
}

void save_carts(const std::filesystem::path& path, const std::vector<Cart>& carts) {
    std::vector<std::string> lines;
    for (const auto& cart : carts) {
        std::ostringstream cart_line;
        cart_line << "C " << std::quoted(cart.id().value) << '\t'
                  << std::quoted(cart.customer_id().value);
        lines.push_back(cart_line.str());
        for (const auto& item : cart.items()) {
            std::ostringstream item_line;
            item_line << "I " << std::quoted(cart.id().value) << '\t'
                      << std::quoted(item.variant_id.value) << '\t'
                      << item.quantity << '\t'
                      << item.unit_price_snapshot.minor_units();
            lines.push_back(item_line.str());
        }
    }
    write_all_lines(path, lines);
}

std::vector<Order> load_orders(const std::filesystem::path& path) {
    struct OrderRecord {
        CustomerId customer_id{CustomerId{""}};
        std::vector<OrderItem> items;
        ShippingAddress address;
        PaymentMethod payment_method{PaymentMethod::Cash};
        OrderStatus status{OrderStatus::Draft};
        PaymentStatus payment_status{PaymentStatus::Unpaid};
        std::optional<std::string> voucher_code;
        Money discount_total{Money::from_minor(0)};
    };

    std::map<std::string, OrderRecord> records;
    for (const auto& line : read_all_lines(path)) {
        std::istringstream input(line);
        std::string tag;
        input >> tag;
        if (tag == "O") {
            std::string order_id;
            std::string customer_id;
            int payment_method = 0;
            int status = 0;
            int payment_status = 0;
            bool has_voucher = false;
            std::string voucher_code;
            std::int64_t discount_minor = 0;
            input >> std::quoted(order_id) >> std::quoted(customer_id) >> payment_method
                  >> status >> payment_status >> has_voucher;
            if (has_voucher) {
                input >> std::quoted(voucher_code);
            }
            input >> discount_minor;
            auto address = deserialize_address(input);

            records.emplace(order_id, OrderRecord{
                CustomerId{customer_id},
                {},
                address,
                enum_from_int<PaymentMethod>(payment_method),
                enum_from_int<OrderStatus>(status),
                enum_from_int<PaymentStatus>(payment_status),
                has_voucher ? std::optional<std::string>{voucher_code} : std::nullopt,
                Money::from_minor(discount_minor)
            });
        } else if (tag == "I") {
            std::string order_id;
            std::string item_id;
            std::string product_id;
            std::string variant_id;
            std::string product_name;
            std::string sku;
            std::string size;
            std::string color;
            int quantity = 0;
            std::int64_t unit_price = 0;
            std::int64_t discount_amount = 0;
            input >> std::quoted(order_id) >> std::quoted(item_id) >> std::quoted(product_id)
                  >> std::quoted(variant_id) >> std::quoted(product_name) >> std::quoted(sku)
                  >> std::quoted(size) >> std::quoted(color) >> quantity >> unit_price >> discount_amount;

            records[order_id].items.push_back(OrderItem{
                OrderItemId{item_id},
                ProductId{product_id},
                VariantId{variant_id},
                product_name,
                sku,
                Size{size},
                Color{color},
                Money::from_minor(unit_price),
                quantity,
                Money::from_minor(discount_amount)
            });
        }
    }

    std::vector<Order> orders;
    for (auto& [order_id, record] : records) {
        orders.push_back(Order::rehydrate(
            OrderId{order_id},
            record.customer_id,
            std::move(record.items),
            record.address,
            record.payment_method,
            record.status,
            record.payment_status,
            std::move(record.voucher_code),
            record.discount_total));
    }
    return orders;
}

void save_orders(const std::filesystem::path& path, const std::vector<Order>& orders) {
    std::vector<std::string> lines;
    for (const auto& order : orders) {
        std::ostringstream order_line;
        order_line << "O " << std::quoted(order.id().value) << '\t'
                   << std::quoted(order.customer_id().value) << '\t'
                   << static_cast<int>(order.payment_method()) << '\t'
                   << static_cast<int>(order.status()) << '\t'
                   << static_cast<int>(order.payment_status()) << '\t'
                   << order.voucher_code().has_value() << '\t';
        if (order.voucher_code().has_value()) {
            order_line << std::quoted(*order.voucher_code()) << '\t';
        }
        order_line << order.discount_total().minor_units() << '\t'
                   << serialize_address(order.shipping_address_snapshot());
        lines.push_back(order_line.str());

        for (const auto& item : order.items()) {
            std::ostringstream item_line;
            item_line << "I " << std::quoted(order.id().value) << '\t'
                      << std::quoted(item.id.value) << '\t'
                      << std::quoted(item.product_id.value) << '\t'
                      << std::quoted(item.variant_id.value) << '\t'
                      << std::quoted(item.product_name) << '\t'
                      << std::quoted(item.sku) << '\t'
                      << std::quoted(item.size.value) << '\t'
                      << std::quoted(item.color.value) << '\t'
                      << item.quantity << '\t'
                      << item.unit_price.minor_units() << '\t'
                      << item.discount_amount.minor_units();
            lines.push_back(item_line.str());
        }
    }
    write_all_lines(path, lines);
}

std::vector<Voucher> load_vouchers(const std::filesystem::path& path) {
    std::vector<Voucher> vouchers;
    for (const auto& line : read_all_lines(path)) {
        std::istringstream input(line);
        std::string tag;
        std::string code;
        int discount_type = 0;
        int discount_value = 0;
        std::int64_t min_order_amount = 0;
        bool has_max_discount = false;
        std::int64_t max_discount = 0;
        std::int64_t start_at = 0;
        std::int64_t end_at = 0;
        bool has_remaining_uses = false;
        int remaining_uses = 0;
        bool is_active = false;
        input >> tag >> std::quoted(code) >> discount_type >> discount_value >> min_order_amount
              >> has_max_discount;
        if (has_max_discount) {
            input >> max_discount;
        }
        input >> start_at >> end_at >> has_remaining_uses;
        if (has_remaining_uses) {
            input >> remaining_uses;
        }
        input >> is_active;
        if (tag == "V") {
            vouchers.emplace_back(
                code,
                enum_from_int<DiscountType>(discount_type),
                discount_value,
                Money::from_minor(min_order_amount),
                has_max_discount ? std::optional<Money>{Money::from_minor(max_discount)} : std::nullopt,
                from_epoch(start_at),
                from_epoch(end_at),
                has_remaining_uses ? std::optional<int>{remaining_uses} : std::nullopt,
                is_active);
        }
    }
    return vouchers;
}

void save_vouchers(const std::filesystem::path& path, const std::vector<Voucher>& vouchers) {
    std::vector<std::string> lines;
    for (const auto& voucher : vouchers) {
        std::ostringstream output;
        output << "V " << std::quoted(voucher.code()) << '\t'
               << static_cast<int>(voucher.discount_type()) << '\t'
               << voucher.discount_value() << '\t'
               << voucher.min_order_amount().minor_units() << '\t'
               << voucher.max_discount_amount().has_value() << '\t';
        if (voucher.max_discount_amount().has_value()) {
            output << voucher.max_discount_amount()->minor_units() << '\t';
        }
        output << to_epoch(voucher.start_at()) << '\t'
               << to_epoch(voucher.end_at()) << '\t'
               << voucher.remaining_uses().has_value() << '\t';
        if (voucher.remaining_uses().has_value()) {
            output << *voucher.remaining_uses() << '\t';
        }
        output << voucher.is_active();
        lines.push_back(output.str());
    }
    write_all_lines(path, lines);
}

std::vector<Account> load_accounts(const std::filesystem::path& path) {
    std::vector<Account> accounts;
    for (const auto& line : read_all_lines(path)) {
        std::istringstream input(line);
        std::string tag;
        std::string id;
        std::string username;
        std::string password_hash;
        int role = 0;
        int status = 0;
        input >> tag >> std::quoted(id) >> std::quoted(username) >> std::quoted(password_hash) >> role >> status;
        if (tag == "A") {
            accounts.emplace_back(
                AccountId{id},
                username,
                password_hash,
                enum_from_int<Role>(role),
                enum_from_int<AccountStatus>(status));
        }
    }
    return accounts;
}

void save_accounts(const std::filesystem::path& path, const std::vector<Account>& accounts) {
    std::vector<std::string> lines;
    for (const auto& account : accounts) {
        std::ostringstream output;
        output << "A " << std::quoted(account.id().value) << '\t'
               << std::quoted(account.username()) << '\t'
               << std::quoted(account.password_hash()) << '\t'
               << static_cast<int>(account.role()) << '\t'
               << static_cast<int>(account.status());
        lines.push_back(output.str());
    }
    write_all_lines(path, lines);
}

std::vector<Customer> load_customers(const std::filesystem::path& path) {
    struct CustomerRecord {
        AccountId account_id{AccountId{""}};
        std::string full_name;
        std::string phone;
        ShippingAddress address;
    };

    std::map<std::string, CustomerRecord> records;
    for (const auto& line : read_all_lines(path)) {
        std::istringstream input(line);
        std::string tag;
        input >> tag;
        if (tag == "C") {
            std::string customer_id;
            std::string account_id;
            std::string full_name;
            std::string phone;
            input >> std::quoted(customer_id) >> std::quoted(account_id) >> std::quoted(full_name) >> std::quoted(phone);
            records.emplace(customer_id, CustomerRecord{
                AccountId{account_id},
                full_name,
                phone,
                deserialize_address(input)
            });
        }
    }

    std::vector<Customer> customers;
    for (auto& [customer_id, record] : records) {
        Customer customer(
            CustomerId{customer_id},
            record.account_id,
            record.full_name,
            record.phone,
            record.address);
        customers.push_back(customer);
    }
    return customers;
}

void save_customers(const std::filesystem::path& path, const std::vector<Customer>& customers) {
    std::vector<std::string> lines;
    for (const auto& customer : customers) {
        std::ostringstream customer_line;
        customer_line << "C " << std::quoted(customer.id().value) << '\t'
                      << std::quoted(customer.account_id().value) << '\t'
                      << std::quoted(customer.full_name()) << '\t'
                      << std::quoted(customer.phone()) << '\t'
                      << serialize_address(customer.default_shipping_address());
        lines.push_back(customer_line.str());
    }
    write_all_lines(path, lines);
}

std::vector<Review> load_reviews(const std::filesystem::path& path) {
    std::vector<Review> reviews;
    for (const auto& line : read_all_lines(path)) {
        std::istringstream input(line);
        std::string tag;
        std::string review_id;
        std::string customer_id;
        std::string product_id;
        bool has_variant = false;
        std::string variant_id;
        int rating = 0;
        std::string comment;
        bool verified_purchase = false;
        input >> tag >> std::quoted(review_id) >> std::quoted(customer_id) >> std::quoted(product_id) >> has_variant;
        if (has_variant) {
            input >> std::quoted(variant_id);
        }
        input >> rating >> std::quoted(comment) >> verified_purchase;
        if (tag == "R") {
            reviews.push_back(Review::rehydrate(
                ReviewId{review_id},
                CustomerId{customer_id},
                ProductId{product_id},
                has_variant ? std::optional<VariantId>{VariantId{variant_id}} : std::nullopt,
                rating,
                comment,
                verified_purchase));
        }
    }
    return reviews;
}

void save_reviews(const std::filesystem::path& path, const std::vector<Review>& reviews) {
    std::vector<std::string> lines;
    for (const auto& review : reviews) {
        std::ostringstream output;
        output << "R " << std::quoted(review.id().value) << '\t'
               << std::quoted(review.customer_id().value) << '\t'
               << std::quoted(review.product_id().value) << '\t'
               << review.variant_id().has_value() << '\t';
        if (review.variant_id().has_value()) {
            output << std::quoted(review.variant_id()->value) << '\t';
        }
        output << review.rating() << '\t'
               << std::quoted(review.comment()) << '\t'
               << review.verified_purchase();
        lines.push_back(output.str());
    }
    write_all_lines(path, lines);
}

std::vector<ReturnRequest> load_returns(const std::filesystem::path& path) {
    std::vector<ReturnRequest> requests;
    for (const auto& line : read_all_lines(path)) {
        std::istringstream input(line);
        std::string tag;
        std::string return_id;
        std::string order_id;
        std::string order_item_id;
        int quantity = 0;
        std::string reason;
        int status = 0;
        input >> tag >> std::quoted(return_id) >> std::quoted(order_id) >> std::quoted(order_item_id)
              >> quantity >> std::quoted(reason) >> status;
        if (tag == "T") {
            requests.push_back(ReturnRequest::rehydrate(
                ReturnId{return_id},
                OrderId{order_id},
                OrderItemId{order_item_id},
                quantity,
                reason,
                enum_from_int<ReturnStatus>(status)));
        }
    }
    return requests;
}

void save_returns(const std::filesystem::path& path, const std::vector<ReturnRequest>& requests) {
    std::vector<std::string> lines;
    for (const auto& request : requests) {
        std::ostringstream output;
        output << "T " << std::quoted(request.id().value) << '\t'
               << std::quoted(request.order_id().value) << '\t'
               << std::quoted(request.order_item_id().value) << '\t'
               << request.quantity() << '\t'
               << std::quoted(request.reason()) << '\t'
               << static_cast<int>(request.status());
        lines.push_back(output.str());
    }
    write_all_lines(path, lines);
}

}  // namespace

FileProductRepository::FileProductRepository(std::filesystem::path path) : path_(std::move(path)) {}

std::optional<Product> FileProductRepository::find_by_id(const ProductId& product_id) const {
    for (const auto& product : load_products(path_)) {
        if (product.id() == product_id) {
            return product;
        }
    }
    return std::nullopt;
}

std::optional<CatalogVariantView> FileProductRepository::find_variant(const VariantId& variant_id) const {
    for (const auto& product : load_products(path_)) {
        const auto* variant = product.find_variant(variant_id);
        if (variant != nullptr) {
            return CatalogVariantView{product.id(), product.name(), *variant};
        }
    }
    return std::nullopt;
}

std::vector<Product> FileProductRepository::list_all() const {
    return load_products(path_);
}

void FileProductRepository::save(const Product& product) {
    auto products = load_products(path_);
    bool replaced = false;
    for (auto& existing : products) {
        if (existing.id() == product.id()) {
            existing = product;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        products.push_back(product);
    }
    save_products(path_, products);
}

FileInventoryRepository::FileInventoryRepository(std::filesystem::path path) : path_(std::move(path)) {}

std::optional<InventoryItem> FileInventoryRepository::find_by_variant_id(const VariantId& variant_id) const {
    for (const auto& item : load_inventory_items(path_)) {
        if (item.variant_id() == variant_id) {
            return item;
        }
    }
    return std::nullopt;
}

std::vector<InventoryItem> FileInventoryRepository::list_all() const {
    return load_inventory_items(path_);
}

void FileInventoryRepository::save(const InventoryItem& item) {
    auto items = load_inventory_items(path_);
    bool replaced = false;
    for (auto& existing : items) {
        if (existing.variant_id() == item.variant_id()) {
            existing = item;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        items.push_back(item);
    }
    save_inventory_items(path_, items);
}

FileCartRepository::FileCartRepository(std::filesystem::path path) : path_(std::move(path)) {}

std::optional<Cart> FileCartRepository::find_by_id(const CartId& cart_id) const {
    for (const auto& cart : load_carts(path_)) {
        if (cart.id() == cart_id) {
            return cart;
        }
    }
    return std::nullopt;
}

std::optional<Cart> FileCartRepository::find_by_customer_id(const CustomerId& customer_id) const {
    for (const auto& cart : load_carts(path_)) {
        if (cart.customer_id() == customer_id) {
            return cart;
        }
    }
    return std::nullopt;
}

void FileCartRepository::save(const Cart& cart) {
    auto carts = load_carts(path_);
    bool replaced = false;
    for (auto& existing : carts) {
        if (existing.id() == cart.id()) {
            existing = cart;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        carts.push_back(cart);
    }
    save_carts(path_, carts);
}

FileOrderRepository::FileOrderRepository(std::filesystem::path path) : path_(std::move(path)) {}

std::optional<Order> FileOrderRepository::find_by_id(const OrderId& order_id) const {
    for (const auto& order : load_orders(path_)) {
        if (order.id() == order_id) {
            return order;
        }
    }
    return std::nullopt;
}

std::vector<Order> FileOrderRepository::find_by_customer_id(const CustomerId& customer_id) const {
    std::vector<Order> orders;
    for (const auto& order : load_orders(path_)) {
        if (order.customer_id() == customer_id) {
            orders.push_back(order);
        }
    }
    return orders;
}

std::vector<Order> FileOrderRepository::list_all() const {
    return load_orders(path_);
}

void FileOrderRepository::save(const Order& order) {
    auto orders = load_orders(path_);
    bool replaced = false;
    for (auto& existing : orders) {
        if (existing.id() == order.id()) {
            existing = order;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        orders.push_back(order);
    }
    save_orders(path_, orders);
}

FileVoucherRepository::FileVoucherRepository(std::filesystem::path path) : path_(std::move(path)) {}

std::optional<Voucher> FileVoucherRepository::find_by_code(std::string_view code) const {
    for (const auto& voucher : load_vouchers(path_)) {
        if (voucher.code() == code) {
            return voucher;
        }
    }
    return std::nullopt;
}

void FileVoucherRepository::save(const Voucher& voucher) {
    auto vouchers = load_vouchers(path_);
    bool replaced = false;
    for (auto& existing : vouchers) {
        if (existing.code() == voucher.code()) {
            existing = voucher;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        vouchers.push_back(voucher);
    }
    save_vouchers(path_, vouchers);
}

FileAccountRepository::FileAccountRepository(std::filesystem::path path) : path_(std::move(path)) {}

std::optional<Account> FileAccountRepository::find_by_id(const AccountId& account_id) const {
    for (const auto& account : load_accounts(path_)) {
        if (account.id() == account_id) {
            return account;
        }
    }
    return std::nullopt;
}

std::optional<Account> FileAccountRepository::find_by_username(const std::string& username) const {
    for (const auto& account : load_accounts(path_)) {
        if (account.username() == username) {
            return account;
        }
    }
    return std::nullopt;
}

void FileAccountRepository::save(const Account& account) {
    auto accounts = load_accounts(path_);
    bool replaced = false;
    for (auto& existing : accounts) {
        if (existing.id() == account.id()) {
            existing = account;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        accounts.push_back(account);
    }
    save_accounts(path_, accounts);
}

FileCustomerRepository::FileCustomerRepository(std::filesystem::path path) : path_(std::move(path)) {}

std::optional<Customer> FileCustomerRepository::find_by_id(const CustomerId& customer_id) const {
    for (const auto& customer : load_customers(path_)) {
        if (customer.id() == customer_id) {
            return customer;
        }
    }
    return std::nullopt;
}

std::optional<Customer> FileCustomerRepository::find_by_account_id(const AccountId& account_id) const {
    for (const auto& customer : load_customers(path_)) {
        if (customer.account_id() == account_id) {
            return customer;
        }
    }
    return std::nullopt;
}

void FileCustomerRepository::save(const Customer& customer) {
    auto customers = load_customers(path_);
    bool replaced = false;
    for (auto& existing : customers) {
        if (existing.id() == customer.id()) {
            existing = customer;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        customers.push_back(customer);
    }
    save_customers(path_, customers);
}

FileReviewRepository::FileReviewRepository(std::filesystem::path path) : path_(std::move(path)) {}

std::vector<Review> FileReviewRepository::find_by_product_id(const ProductId& product_id) const {
    std::vector<Review> result;
    for (const auto& review : load_reviews(path_)) {
        if (review.product_id() == product_id) {
            result.push_back(review);
        }
    }
    return result;
}

std::vector<Review> FileReviewRepository::find_by_customer_id(const CustomerId& customer_id) const {
    std::vector<Review> result;
    for (const auto& review : load_reviews(path_)) {
        if (review.customer_id() == customer_id) {
            result.push_back(review);
        }
    }
    return result;
}

void FileReviewRepository::save(const Review& review) {
    auto reviews = load_reviews(path_);
    bool replaced = false;
    for (auto& existing : reviews) {
        if (existing.id() == review.id()) {
            existing = review;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        reviews.push_back(review);
    }
    save_reviews(path_, reviews);
}

FileReturnRepository::FileReturnRepository(std::filesystem::path path) : path_(std::move(path)) {}

std::optional<ReturnRequest> FileReturnRepository::find_by_id(const ReturnId& return_id) const {
    for (const auto& request : load_returns(path_)) {
        if (request.id() == return_id) {
            return request;
        }
    }
    return std::nullopt;
}

std::vector<ReturnRequest> FileReturnRepository::find_by_order_id(const OrderId& order_id) const {
    std::vector<ReturnRequest> result;
    for (const auto& request : load_returns(path_)) {
        if (request.order_id() == order_id) {
            result.push_back(request);
        }
    }
    return result;
}

void FileReturnRepository::save(const ReturnRequest& request) {
    auto requests = load_returns(path_);
    bool replaced = false;
    for (auto& existing : requests) {
        if (existing.id() == request.id()) {
            existing = request;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        requests.push_back(request);
    }
    save_returns(path_, requests);
}

}  // namespace fashion_store::infrastructure::persistence::file
