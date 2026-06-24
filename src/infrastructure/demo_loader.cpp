#include "infrastructure/demo_loader.hpp"
#include "application/cart/cart_application_service.hpp"
#include "application/order/order_application_service.hpp"
#include "domain/customer/customer.hpp"
#include "domain/identity/account.hpp"
#include "domain/pricing/voucher.hpp"
#include "domain/order/order.hpp"
#include "domain/catalog/product.hpp"
#include "domain/inventory/inventory_item.hpp"
#include "domain/shared/types.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <cctype>

namespace fashion_store::infrastructure {

using fashion_store::domain::catalog::Product;
using fashion_store::domain::catalog::Category;
using fashion_store::domain::catalog::ProductVariantDraft;
using fashion_store::domain::catalog::Size;
using fashion_store::domain::catalog::Color;
using fashion_store::domain::inventory::InventoryItem;
using fashion_store::domain::shared::ProductId;
using fashion_store::domain::shared::VariantId;
using fashion_store::domain::shared::Money;

using fashion_store::domain::identity::Account;
using fashion_store::domain::identity::AccountId;
using fashion_store::domain::identity::Role;
using fashion_store::domain::identity::AccountStatus;

using fashion_store::domain::customer::Customer;
using fashion_store::domain::customer::CustomerId;
using fashion_store::domain::shared::ShippingAddress;

using fashion_store::domain::pricing::Voucher;
using fashion_store::domain::pricing::DiscountType;

using fashion_store::domain::shared::OrderId;
using fashion_store::domain::shared::CartId;
using fashion_store::domain::order::PaymentMethod;
using fashion_store::domain::order::OrderStatus;
using fashion_store::application::order::PlaceOrderCommand;

namespace {

enum class TokenType {
    CurlyOpen,
    CurlyClose,
    BracketOpen,
    BracketClose,
    Colon,
    Comma,
    String,
    Number,
    Boolean,
    End
};

struct Token {
    TokenType type;
    std::string value;
};

std::vector<Token> tokenize(const std::string& json) {
    std::vector<Token> tokens;
    size_t i = 0;
    while (i < json.size()) {
        char c = json[i];
        if (std::isspace(static_cast<unsigned char>(c))) {
            i++;
            continue;
        }
        if (c == '{') {
            tokens.push_back({TokenType::CurlyOpen, "{"});
            i++;
        } else if (c == '}') {
            tokens.push_back({TokenType::CurlyClose, "}"});
            i++;
        } else if (c == '[') {
            tokens.push_back({TokenType::BracketOpen, "["});
            i++;
        } else if (c == ']') {
            tokens.push_back({TokenType::BracketClose, "]"});
            i++;
        } else if (c == ':') {
            tokens.push_back({TokenType::Colon, ":"});
            i++;
        } else if (c == ',') {
            tokens.push_back({TokenType::Comma, ","});
            i++;
        } else if (c == '"') {
            std::string str;
            i++;
            while (i < json.size() && json[i] != '"') {
                if (json[i] == '\\' && i + 1 < json.size()) {
                    i++;
                    char esc = json[i];
                    if (esc == 'n') str += '\n';
                    else if (esc == 't') str += '\t';
                    else if (esc == 'r') str += '\r';
                    else str += esc;
                } else {
                    str += json[i];
                }
                i++;
            }
            if (i < json.size()) i++;
            tokens.push_back({TokenType::String, str});
        } else if (std::isdigit(static_cast<unsigned char>(c)) || c == '-') {
            std::string num;
            while (i < json.size() && (std::isdigit(static_cast<unsigned char>(json[i])) || json[i] == '.' || json[i] == '-')) {
                num += json[i];
                i++;
            }
            tokens.push_back({TokenType::Number, num});
        } else if (json.compare(i, 4, "true") == 0) {
            tokens.push_back({TokenType::Boolean, "true"});
            i += 4;
        } else if (json.compare(i, 5, "false") == 0) {
            tokens.push_back({TokenType::Boolean, "false"});
            i += 5;
        } else {
            i++;
        }
    }
    tokens.push_back({TokenType::End, ""});
    return tokens;
}

struct JsonValue {
    enum class Type { Null, Object, Array, String, Number, Boolean };
    Type type = Type::Null;
    std::string value;
    std::unordered_map<std::string, JsonValue> obj;
    std::vector<JsonValue> arr;
};

JsonValue parse_value(const std::vector<Token>& tokens, size_t& index) {
    if (index >= tokens.size()) return {};
    const auto& tok = tokens[index];
    if (tok.type == TokenType::String) {
        JsonValue v;
        v.type = JsonValue::Type::String;
        v.value = tok.value;
        index++;
        return v;
    }
    if (tok.type == TokenType::Number) {
        JsonValue v;
        v.type = JsonValue::Type::Number;
        v.value = tok.value;
        index++;
        return v;
    }
    if (tok.type == TokenType::Boolean) {
        JsonValue v;
        v.type = JsonValue::Type::Boolean;
        v.value = tok.value;
        index++;
        return v;
    }
    if (tok.type == TokenType::CurlyOpen) {
        index++;
        JsonValue v;
        v.type = JsonValue::Type::Object;
        while (index < tokens.size() && tokens[index].type != TokenType::CurlyClose) {
            if (tokens[index].type != TokenType::String) {
                index++;
                continue;
            }
            std::string key = tokens[index].value;
            index++;
            if (index < tokens.size() && tokens[index].type == TokenType::Colon) {
                index++;
            }
            v.obj[key] = parse_value(tokens, index);
            if (index < tokens.size() && tokens[index].type == TokenType::Comma) {
                index++;
            }
        }
        if (index < tokens.size() && tokens[index].type == TokenType::CurlyClose) {
            index++;
        }
        return v;
    }
    if (tok.type == TokenType::BracketOpen) {
        index++;
        JsonValue v;
        v.type = JsonValue::Type::Array;
        while (index < tokens.size() && tokens[index].type != TokenType::BracketClose) {
            v.arr.push_back(parse_value(tokens, index));
            if (index < tokens.size() && tokens[index].type == TokenType::Comma) {
                index++;
            }
        }
        if (index < tokens.size() && tokens[index].type == TokenType::BracketClose) {
            index++;
        }
        return v;
    }
    return {};
}

JsonValue load_json_file(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        std::cerr << "Warning: Demo configuration file not found: " << path << ". Skipping seeding for this context." << std::endl;
        return {};
    }
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "Error: Failed to open demo configuration file: " << path << std::endl;
        return {};
    }
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    auto tokens = tokenize(content);
    size_t index = 0;
    return parse_value(tokens, index);
}

Category parse_category(const std::string& cat_str) {
    if (cat_str == "Tops") return Category::Tops;
    if (cat_str == "Bottoms") return Category::Bottoms;
    if (cat_str == "Outerwear") return Category::Outerwear;
    if (cat_str == "Dresses") return Category::Dresses;
    if (cat_str == "Shoes") return Category::Shoes;
    return Category::Accessories;
}

Role parse_role(const std::string& role_str) {
    if (role_str == "Staff") return Role::Staff;
    if (role_str == "Admin") return Role::Admin;
    return Role::Customer;
}

DiscountType parse_discount_type(const std::string& type_str) {
    if (type_str == "FixedAmount") return DiscountType::FixedAmount;
    return DiscountType::Percentage;
}

PaymentMethod parse_payment_method(const std::string& method_str) {
    if (method_str == "BankTransfer") return PaymentMethod::BankTransfer;
    if (method_str == "Cash") return PaymentMethod::Cash;
    return PaymentMethod::EWallet;
}

ShippingAddress parse_address(const JsonValue& addr_val) {
    ShippingAddress addr;
    if (addr_val.type != JsonValue::Type::Object) return addr;
    
    auto rn = addr_val.obj.find("recipient_name");
    auto ph = addr_val.obj.find("phone");
    auto l1 = addr_val.obj.find("line1");
    auto l2 = addr_val.obj.find("line2");
    auto wd = addr_val.obj.find("ward");
    auto ds = addr_val.obj.find("district");
    auto cy = addr_val.obj.find("city");
    auto ct = addr_val.obj.find("country");

    if (rn != addr_val.obj.end()) addr.recipient_name = rn->second.value;
    if (ph != addr_val.obj.end()) addr.phone = ph->second.value;
    if (l1 != addr_val.obj.end()) addr.line1 = l1->second.value;
    if (l2 != addr_val.obj.end()) addr.line2 = l2->second.value;
    if (wd != addr_val.obj.end()) addr.ward = wd->second.value;
    if (ds != addr_val.obj.end()) addr.district = ds->second.value;
    if (cy != addr_val.obj.end()) addr.city = cy->second.value;
    if (ct != addr_val.obj.end()) addr.country = ct->second.value;

    return addr;
}

void save_storefront_metadata(const std::filesystem::path& path, const std::unordered_map<std::string, std::vector<std::string>>& metadata) {
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream out(path);
    if (!out.is_open()) return;
    out << "{\n";
    bool first = true;
    for (const auto& [prod_id, images] : metadata) {
        if (!first) out << ",\n";
        first = false;
        out << "  \"" << prod_id << "\": [\n";
        for (size_t i = 0; i < images.size(); ++i) {
            out << "    \"" << images[i] << "\"";
            if (i + 1 < images.size()) out << ",";
            out << "\n";
        }
        out << "  ]";
    }
    out << "\n}\n";
}

} // namespace

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
) {
    // 1. PRODUCTS & INVENTORY
    auto products_path = demo_dir / "products.json";
    JsonValue products_root = load_json_file(products_path);
    std::unordered_map<std::string, std::vector<std::string>> storefront_images;

    if (products_root.type == JsonValue::Type::Array) {
        for (const auto& item_val : products_root.arr) {
            if (item_val.type != JsonValue::Type::Object) continue;

            auto id_it = item_val.obj.find("id");
            auto name_it = item_val.obj.find("name");
            auto cat_it = item_val.obj.find("category");
            auto desc_it = item_val.obj.find("description");
            auto coll_it = item_val.obj.find("collection");
            auto variants_it = item_val.obj.find("variants");
            auto images_it = item_val.obj.find("images");

            if (id_it == item_val.obj.end() || name_it == item_val.obj.end() ||
                cat_it == item_val.obj.end() || desc_it == item_val.obj.end() ||
                variants_it == item_val.obj.end()) {
                continue;
            }

            std::string product_id_str = id_it->second.value;
            std::string name = name_it->second.value;
            Category category = parse_category(cat_it->second.value);
            std::string description = desc_it->second.value;
            std::string collection = (coll_it != item_val.obj.end()) ? coll_it->second.value : "";

            ProductId product_id{product_id_str};

            // Storefront images extraction
            if (images_it != item_val.obj.end() && images_it->second.type == JsonValue::Type::Array) {
                std::vector<std::string> urls;
                for (const auto& url_val : images_it->second.arr) {
                    if (url_val.type == JsonValue::Type::String) {
                        urls.push_back(url_val.value);
                    }
                }
                if (!urls.empty()) {
                    storefront_images[product_id_str] = urls;
                }
            }

            if (!product_repository.find_by_id(product_id).has_value()) {
                Product product(product_id, name, category, description, collection);

                if (variants_it->second.type == JsonValue::Type::Array) {
                    for (const auto& var_val : variants_it->second.arr) {
                        if (var_val.type != JsonValue::Type::Object) continue;

                        auto var_id_it = var_val.obj.find("id");
                        auto sku_it = var_val.obj.find("sku");
                        auto size_it = var_val.obj.find("size");
                        auto color_it = var_val.obj.find("color");
                        auto price_it = var_val.obj.find("price");
                        auto qty_it = var_val.obj.find("quantity");

                        if (var_id_it == var_val.obj.end() || sku_it == var_val.obj.end() ||
                            size_it == var_val.obj.end() || color_it == var_val.obj.end() ||
                            price_it == var_val.obj.end()) {
                            continue;
                        }

                        std::string var_id_str = var_id_it->second.value;
                        std::string sku = sku_it->second.value;
                        std::string size = size_it->second.value;
                        std::string color = color_it->second.value;
                        std::int64_t price_minor = 0;
                        try {
                            price_minor = std::stoll(price_it->second.value);
                        } catch (...) {}

                        (void)product.add_variant(ProductVariantDraft{
                            VariantId{var_id_str},
                            sku,
                            Size{size},
                            Color{color},
                            Money::from_minor(price_minor)
                        });

                        VariantId variant_id{var_id_str};
                        if (!inventory_repository.find_by_variant_id(variant_id).has_value()) {
                            int quantity = 0;
                            if (qty_it != var_val.obj.end()) {
                                try {
                                    quantity = std::stoi(qty_it->second.value);
                                } catch (...) {}
                            }
                            inventory_repository.save(InventoryItem(variant_id, quantity));
                        }
                    }
                }
                product_repository.save(product);
            } else {
                if (variants_it->second.type == JsonValue::Type::Array) {
                    for (const auto& var_val : variants_it->second.arr) {
                        if (var_val.type != JsonValue::Type::Object) continue;

                        auto var_id_it = var_val.obj.find("id");
                        auto qty_it = var_val.obj.find("quantity");

                        if (var_id_it == var_val.obj.end()) continue;

                        VariantId variant_id{var_id_it->second.value};
                        if (!inventory_repository.find_by_variant_id(variant_id).has_value()) {
                            int quantity = 0;
                            if (qty_it != var_val.obj.end()) {
                                try {
                                    quantity = std::stoi(qty_it->second.value);
                                } catch (...) {}
                            }
                            inventory_repository.save(InventoryItem(variant_id, quantity));
                        }
                    }
                }
            }
        }
    }

    // Save storefront images metadata if product_storefront.json is not present/empty
    if (!storefront_images.empty() && (!std::filesystem::exists(product_storefront_path) || std::filesystem::is_empty(product_storefront_path))) {
        save_storefront_metadata(product_storefront_path, storefront_images);
    }

    // 2. ACCOUNTS & CUSTOMER PROFILES
    auto accounts_path = demo_dir / "accounts.json";
    JsonValue accounts_root = load_json_file(accounts_path);

    if (accounts_root.type == JsonValue::Type::Array) {
        for (const auto& acc_val : accounts_root.arr) {
            if (acc_val.type != JsonValue::Type::Object) continue;

            auto id_it = acc_val.obj.find("id");
            auto user_it = acc_val.obj.find("username");
            auto hash_it = acc_val.obj.find("password_hash");
            auto role_it = acc_val.obj.find("role");
            auto profile_it = acc_val.obj.find("customer_profile");

            if (id_it == acc_val.obj.end() || user_it == acc_val.obj.end() ||
                hash_it == acc_val.obj.end() || role_it == acc_val.obj.end()) {
                continue;
            }

            std::string acc_id_str = id_it->second.value;
            std::string username = user_it->second.value;
            std::string password_hash = hash_it->second.value;
            Role role = parse_role(role_it->second.value);

            AccountId account_id{acc_id_str};
            if (!account_repository.find_by_id(account_id).has_value()) {
                account_repository.save(Account(
                    account_id,
                    username,
                    password_hash,
                    role,
                    AccountStatus::Active
                ));
            }

            if (profile_it != acc_val.obj.end() && profile_it->second.type == JsonValue::Type::Object) {
                const auto& prof = profile_it->second;
                auto prof_id_it = prof.obj.find("id");
                auto name_it = prof.obj.find("name");
                auto phone_it = prof.obj.find("phone");
                auto addr_it = prof.obj.find("address");

                if (prof_id_it != prof.obj.end() && name_it != prof.obj.end() && phone_it != prof.obj.end() && addr_it != prof.obj.end()) {
                    CustomerId customer_id{prof_id_it->second.value};
                    if (!customer_repository.find_by_id(customer_id).has_value()) {
                        ShippingAddress addr = parse_address(addr_it->second);
                        customer_repository.save(Customer(
                            customer_id,
                            account_id,
                            name_it->second.value,
                            phone_it->second.value,
                            addr
                        ));
                    }
                }
            }
        }
    }

    // 3. VOUCHERS
    auto vouchers_path = demo_dir / "vouchers.json";
    JsonValue vouchers_root = load_json_file(vouchers_path);

    if (vouchers_root.type == JsonValue::Type::Array) {
        for (const auto& v_val : vouchers_root.arr) {
            if (v_val.type != JsonValue::Type::Object) continue;

            auto code_it = v_val.obj.find("code");
            auto type_it = v_val.obj.find("discount_type");
            auto val_it = v_val.obj.find("discount_value");
            auto min_it = v_val.obj.find("min_purchase");
            auto max_it = v_val.obj.find("max_discount");
            auto days_it = v_val.obj.find("days_valid");
            auto limit_it = v_val.obj.find("usage_limit");

            if (code_it == v_val.obj.end() || type_it == v_val.obj.end() ||
                val_it == v_val.obj.end() || min_it == v_val.obj.end() ||
                max_it == v_val.obj.end() || days_it == v_val.obj.end() ||
                limit_it == v_val.obj.end()) {
                continue;
            }

            std::string code = code_it->second.value;
            DiscountType discount_type = parse_discount_type(type_it->second.value);
            int discount_value = 0;
            std::int64_t min_purchase = 0;
            std::int64_t max_discount = 0;
            int days_valid = 30;
            int usage_limit = 100;

            try {
                discount_value = std::stoi(val_it->second.value);
                min_purchase = std::stoll(min_it->second.value);
                max_discount = std::stoll(max_it->second.value);
                days_valid = std::stoi(days_it->second.value);
                usage_limit = std::stoi(limit_it->second.value);
            } catch (...) {}

            if (!voucher_repository.find_by_code(code).has_value()) {
                const auto now = std::chrono::system_clock::now();
                voucher_repository.save(Voucher(
                    code,
                    discount_type,
                    discount_value,
                    Money::from_minor(min_purchase),
                    Money::from_minor(max_discount),
                    now - std::chrono::hours(24),
                    now + std::chrono::hours(24 * days_valid),
                    usage_limit,
                    true
                ));
            }
        }
    }

    // 4. DEMO ORDERS
    auto orders_path = demo_dir / "orders.json";
    JsonValue orders_root = load_json_file(orders_path);

    if (order_repository.list_all().empty() && orders_root.type == JsonValue::Type::Array) {
        for (const auto& ord_val : orders_root.arr) {
            if (ord_val.type != JsonValue::Type::Object) continue;

            auto id_it = ord_val.obj.find("id");
            auto cust_id_it = ord_val.obj.find("customer_id");
            auto cart_id_it = ord_val.obj.find("cart_id");
            auto pay_it = ord_val.obj.find("payment_method");
            auto voucher_it = ord_val.obj.find("voucher_code");
            auto items_it = ord_val.obj.find("items");
            auto workflow_it = ord_val.obj.find("workflow");

            if (id_it == ord_val.obj.end() || cust_id_it == ord_val.obj.end() ||
                cart_id_it == ord_val.obj.end() || pay_it == ord_val.obj.end() ||
                items_it == ord_val.obj.end() || workflow_it == ord_val.obj.end()) {
                continue;
            }

            OrderId order_id{id_it->second.value};
            CustomerId customer_id{cust_id_it->second.value};
            CartId cart_id{cart_id_it->second.value};
            PaymentMethod payment_method = parse_payment_method(pay_it->second.value);
            std::string voucher_code = (voucher_it != ord_val.obj.end()) ? voucher_it->second.value : "";

            // Get customer address to place the order
            ShippingAddress shipping_address;
            auto customer = customer_repository.find_by_id(customer_id);
            if (customer.has_value()) {
                shipping_address = customer->default_shipping_address();
            } else {
                // Fallback default address
                shipping_address = ShippingAddress{
                    "Nguyen Van A", "0900000000", "12 Nguyen Hue", "", "Ben Nghe", "District 1", "Ho Chi Minh City", "Vietnam"
                };
            }

            // Add items to cart
            if (items_it->second.type == JsonValue::Type::Array) {
                for (const auto& item : items_it->second.arr) {
                    if (item.type != JsonValue::Type::Object) continue;
                    auto var_it = item.obj.find("variant_id");
                    auto qty_it = item.obj.find("quantity");
                    if (var_it != item.obj.end() && qty_it != item.obj.end()) {
                        int quantity = 1;
                        try {
                            quantity = std::stoi(qty_it->second.value);
                        } catch (...) {}
                        (void)cart_svc.add_item(cart_id, customer_id, VariantId{var_it->second.value}, quantity);
                    }
                }
            }

            // Place order
            PlaceOrderCommand cmd{
                order_id,
                cart_id,
                customer_id,
                shipping_address,
                payment_method,
                voucher_code.empty() ? std::nullopt : std::optional<std::string>{voucher_code}
            };

            auto place_result = order_svc.place_order(cmd);
            if (!place_result) {
                std::cerr << "Seeding order " << order_id.value << " place failed\n";
                continue;
            }

            // Advance status through workflow
            if (workflow_it->second.type == JsonValue::Type::Array) {
                for (const auto& wf_step : workflow_it->second.arr) {
                    if (wf_step.type != JsonValue::Type::String) continue;
                    std::string step = wf_step.value;

                    if (step == "Paid") {
                        (void)order_svc.mark_order_paid(order_id);
                    } else if (step == "Packed") {
                        (void)order_svc.advance_order_status(order_id, OrderStatus::Packed);
                    } else if (step == "Shipped") {
                        (void)order_svc.advance_order_status(order_id, OrderStatus::Shipped);
                    } else if (step == "Delivered") {
                        (void)order_svc.advance_order_status(order_id, OrderStatus::Delivered);
                    } else if (step == "Completed") {
                        (void)order_svc.advance_order_status(order_id, OrderStatus::Completed);
                    }
                }
            }
        }
    }
}

} // namespace fashion_store::infrastructure
