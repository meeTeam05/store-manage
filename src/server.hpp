#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "httplib.h"

#include "application/cart/cart_application_service.hpp"
#include "application/catalog/catalog_application_service.hpp"
#include "application/customer/customer_application_service.hpp"
#include "application/identity/auth_application_service.hpp"
#include "application/order/order_application_service.hpp"
#include "application/payment/payment_application_service.hpp"
#include "application/report/report_application_service.hpp"
#include "application/returns/returns_application_service.hpp"
#include "application/review/review_application_service.hpp"
#include "application/shipping/shipping_application_service.hpp"
#include "application/staff/return_management_service.hpp"
#include "application/staff/store_management_service.hpp"
#include "domain/catalog/product.hpp"
#include "domain/customer/customer.hpp"
#include "domain/order/order.hpp"
#include "domain/pricing/voucher.hpp"
#include "domain/returns/return_request.hpp"
#include "domain/review/review.hpp"
#include "domain/shipping/shipment.hpp"

namespace fashion_store::server {

// ── JSON builder ─────────────────────────────────────────────────────────────

static std::string j_esc(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    out += '"';
    for (unsigned char c : s) {
        if (c == '"')       out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20)  out += ' ';
        else                out += static_cast<char>(c);
    }
    out += '"';
    return out;
}

static std::string j_num(long long n)  { return std::to_string(n); }
static std::string j_bool(bool b)      { return b ? "true" : "false"; }
static std::string j_null()            { return "null"; }
static std::string j_str(const std::string& s) { return j_esc(s); }

static std::string j_obj(
    std::initializer_list<std::pair<const char*, std::string>> fields) {
    std::string out = "{";
    bool first = true;
    for (auto& [k, v] : fields) {
        if (!first) out += ',';
        out += '"'; out += k; out += "\":"; out += v;
        first = false;
    }
    out += '}';
    return out;
}

static std::string j_arr(const std::vector<std::string>& items) {
    std::string out = "[";
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i) out += ',';
        out += items[i];
    }
    out += ']';
    return out;
}

static std::string ok_json(const std::string& data) {
    return "{\"ok\":true,\"data\":" + data + "}";
}

static std::string err_json(const std::string& msg) {
    return "{\"ok\":false,\"error\":" + j_str(msg) + "}";
}

struct ProductStorefrontMetadata {
    std::map<std::string, std::vector<std::string>> images_by_product;
};

static void skip_json_ws(const std::string& source, std::size_t& position) {
    while (position < source.size() &&
           std::isspace(static_cast<unsigned char>(source[position]))) {
        ++position;
    }
}

static std::optional<std::string> parse_json_string_token(
    const std::string& source,
    std::size_t& position) {
    skip_json_ws(source, position);
    if (position >= source.size() || source[position] != '"') {
        return std::nullopt;
    }
    ++position;

    std::string value;
    while (position < source.size()) {
        const char current = source[position++];
        if (current == '"') {
            return value;
        }
        if (current != '\\') {
            value.push_back(current);
            continue;
        }
        if (position >= source.size()) {
            return std::nullopt;
        }
        const char escaped = source[position++];
        switch (escaped) {
            case '"': value.push_back('"'); break;
            case '\\': value.push_back('\\'); break;
            case '/': value.push_back('/'); break;
            case 'n': value.push_back('\n'); break;
            case 'r': value.push_back('\r'); break;
            case 't': value.push_back('\t'); break;
            default: value.push_back(escaped); break;
        }
    }
    return std::nullopt;
}

static std::optional<std::vector<std::string>> parse_json_string_array(
    const std::string& source,
    std::size_t& position) {
    skip_json_ws(source, position);
    if (position >= source.size() || source[position] != '[') {
        return std::nullopt;
    }
    ++position;

    std::vector<std::string> values;
    while (true) {
        skip_json_ws(source, position);
        if (position >= source.size()) {
            return std::nullopt;
        }
        if (source[position] == ']') {
            ++position;
            return values;
        }

        auto value = parse_json_string_token(source, position);
        if (!value.has_value()) {
            return std::nullopt;
        }
        values.push_back(*value);

        skip_json_ws(source, position);
        if (position >= source.size()) {
            return std::nullopt;
        }
        if (source[position] == ',') {
            ++position;
            continue;
        }
        if (source[position] == ']') {
            ++position;
            return values;
        }
        return std::nullopt;
    }
}

static ProductStorefrontMetadata load_product_storefront_metadata(
    const std::filesystem::path& path) {
    ProductStorefrontMetadata metadata;
    std::ifstream input(path);
    if (!input.is_open()) {
        return metadata;
    }

    const std::string content{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
    if (content.empty()) {
        return metadata;
    }

    std::size_t position = 0;
    skip_json_ws(content, position);
    if (position >= content.size() || content[position] != '{') {
        return metadata;
    }
    ++position;

    while (true) {
        skip_json_ws(content, position);
        if (position >= content.size()) {
            return ProductStorefrontMetadata{};
        }
        if (content[position] == '}') {
            return metadata;
        }

        auto product_id = parse_json_string_token(content, position);
        if (!product_id.has_value()) {
            return ProductStorefrontMetadata{};
        }

        skip_json_ws(content, position);
        if (position >= content.size() || content[position] != ':') {
            return ProductStorefrontMetadata{};
        }
        ++position;

        auto images = parse_json_string_array(content, position);
        if (!images.has_value()) {
            return ProductStorefrontMetadata{};
        }
        metadata.images_by_product[*product_id] = std::move(*images);

        skip_json_ws(content, position);
        if (position >= content.size()) {
            return ProductStorefrontMetadata{};
        }
        if (content[position] == ',') {
            ++position;
            continue;
        }
        if (content[position] == '}') {
            return metadata;
        }
        return ProductStorefrontMetadata{};
    }
}

static void save_product_storefront_metadata(
    const std::filesystem::path& path,
    const ProductStorefrontMetadata& metadata) {
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream output(path, std::ios::trunc);
    output << "{\n";
    bool first_product = true;
    for (const auto& [product_id, images] : metadata.images_by_product) {
        if (!first_product) {
            output << ",\n";
        }
        output << "  " << j_str(product_id) << ": [";
        for (std::size_t index = 0; index < images.size(); ++index) {
            output << (index == 0 ? "\n" : ",\n");
            output << "    " << j_str(images[index]);
        }
        if (!images.empty()) {
            output << '\n' << "  ";
        }
        output << "]";
        first_product = false;
    }
    if (!metadata.images_by_product.empty()) {
        output << '\n';
    }
    output << "}\n";
}

static std::string trim_copy(std::string value) {
    while (!value.empty() &&
           std::isspace(static_cast<unsigned char>(value.front()))) {
        value.erase(value.begin());
    }
    while (!value.empty() &&
           std::isspace(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
    return value;
}

static std::vector<std::string> parse_image_urls_text(const std::string& text) {
    std::string normalized = text;
    for (std::size_t position = 0; (position = normalized.find("\\r\\n", position)) != std::string::npos;) {
        normalized.replace(position, 4, "\n");
    }
    for (std::size_t position = 0; (position = normalized.find("\\n", position)) != std::string::npos;) {
        normalized.replace(position, 2, "\n");
    }
    for (std::size_t position = 0; (position = normalized.find("\\r", position)) != std::string::npos;) {
        normalized.replace(position, 2, "\n");
    }

    std::vector<std::string> images;
    std::istringstream input(normalized);
    std::string line;
    while (std::getline(input, line)) {
        auto cleaned = trim_copy(line);
        if (cleaned.empty()) {
            continue;
        }
        if (std::find(images.begin(), images.end(), cleaned) == images.end()) {
            images.push_back(cleaned);
        }
    }
    return images;
}

// ── Simple flat JSON body parser ──────────────────────────────────────────────

struct BodyMap {
    std::map<std::string, std::string> fields;

    std::string str(const std::string& k, const std::string& def = "") const {
        auto it = fields.find(k);
        return it != fields.end() ? it->second : def;
    }
    long long num(const std::string& k, long long def = 0) const {
        auto it = fields.find(k);
        if (it == fields.end()) return def;
        try { return std::stoll(it->second); } catch (...) { return def; }
    }
    bool has(const std::string& k) const { return fields.count(k) > 0; }
};

static BodyMap parse_body(const std::string& body) {
    BodyMap result;
    std::size_t pos = 0;
    while (pos < body.size()) {
        auto ks = body.find('"', pos);
        if (ks == std::string::npos) break;
        auto ke = body.find('"', ks + 1);
        if (ke == std::string::npos) break;
        std::string key = body.substr(ks + 1, ke - ks - 1);

        auto colon = body.find(':', ke);
        if (colon == std::string::npos) break;
        std::size_t vs = colon + 1;
        while (vs < body.size() && std::isspace(static_cast<unsigned char>(body[vs]))) ++vs;
        if (vs >= body.size()) break;

        std::string value;
        if (body[vs] == '"') {
            std::size_t ve = vs + 1;
            while (ve < body.size()) {
                if (body[ve] == '\\') { ++ve; ++ve; continue; }
                if (body[ve] == '"') break;
                ++ve;
            }
            value = body.substr(vs + 1, ve - vs - 1);
            pos = ve + 1;
        } else if (body[vs] == 't') { value = "true";  pos = vs + 4; }
        else if  (body[vs] == 'f') { value = "false"; pos = vs + 5; }
        else if  (body[vs] == 'n') { value = "null";  pos = vs + 4; }
        else {
            std::size_t ve = vs;
            while (ve < body.size() && (std::isdigit(static_cast<unsigned char>(body[ve]))
                   || body[ve] == '-' || body[ve] == '.')) ++ve;
            value = body.substr(vs, ve - vs);
            pos = ve;
        }
        if (key != "password_hash" && key != "passwordHash") {
            result.fields[key] = value;
        } else {
            result.fields[key] = value;
        }
    }
    return result;
}

// ── Enum helpers ──────────────────────────────────────────────────────────────

using namespace fashion_store::domain::catalog;
using namespace fashion_store::domain::order;
using namespace fashion_store::domain::returns;
using namespace fashion_store::domain::shipping;
using namespace fashion_store::domain::shared;

static std::string cat_str(Category c) {
    switch (c) {
        case Category::Tops:        return "Tops";
        case Category::Bottoms:     return "Bottoms";
        case Category::Outerwear:   return "Outerwear";
        case Category::Dresses:     return "Dresses";
        case Category::Shoes:       return "Shoes";
        case Category::Accessories: return "Accessories";
    }
    return "Unknown";
}

static Category cat_from(const std::string& s) {
    if (s == "Tops")        return Category::Tops;
    if (s == "Bottoms")     return Category::Bottoms;
    if (s == "Outerwear")   return Category::Outerwear;
    if (s == "Dresses")     return Category::Dresses;
    if (s == "Shoes")       return Category::Shoes;
    return Category::Accessories;
}

static std::string pstatus_str(ProductStatus s) {
    switch (s) {
        case ProductStatus::Draft:        return "Draft";
        case ProductStatus::Active:       return "Active";
        case ProductStatus::Discontinued: return "Discontinued";
    }
    return "Unknown";
}

static ProductStatus pstatus_from(const std::string& s) {
    if (s == "Draft")        return ProductStatus::Draft;
    if (s == "Discontinued") return ProductStatus::Discontinued;
    return ProductStatus::Active;
}

static std::string ostatus_str(OrderStatus s) {
    switch (s) {
        case OrderStatus::Draft:          return "Draft";
        case OrderStatus::AwaitingPayment:return "AwaitingPayment";
        case OrderStatus::Paid:           return "Paid";
        case OrderStatus::Packed:         return "Packed";
        case OrderStatus::Shipped:        return "Shipped";
        case OrderStatus::Delivered:      return "Delivered";
        case OrderStatus::Completed:      return "Completed";
        case OrderStatus::Cancelled:      return "Cancelled";
        case OrderStatus::Returned:       return "Returned";
    }
    return "Unknown";
}

static OrderStatus ostatus_from(const std::string& s) {
    if (s == "Packed")    return OrderStatus::Packed;
    if (s == "Shipped")   return OrderStatus::Shipped;
    if (s == "Delivered") return OrderStatus::Delivered;
    if (s == "Completed") return OrderStatus::Completed;
    return OrderStatus::Cancelled;
}

static std::string paystatus_str(PaymentStatus s) {
    switch (s) {
        case PaymentStatus::Unpaid:   return "Unpaid";
        case PaymentStatus::Pending:  return "Pending";
        case PaymentStatus::Paid:     return "Paid";
        case PaymentStatus::Failed:   return "Failed";
        case PaymentStatus::Refunded: return "Refunded";
    }
    return "Unknown";
}

static std::string paymethod_str(PaymentMethod m) {
    switch (m) {
        case PaymentMethod::Cash:         return "Cash";
        case PaymentMethod::BankTransfer: return "BankTransfer";
        case PaymentMethod::EWallet:      return "EWallet";
    }
    return "Cash";
}

static PaymentMethod paymethod_from(const std::string& s) {
    if (s == "BankTransfer") return PaymentMethod::BankTransfer;
    if (s == "EWallet")      return PaymentMethod::EWallet;
    return PaymentMethod::Cash;
}

static std::string retstat_str(ReturnStatus s) {
    switch (s) {
        case ReturnStatus::Requested: return "Requested";
        case ReturnStatus::Approved:  return "Approved";
        case ReturnStatus::Rejected:  return "Rejected";
        case ReturnStatus::Restocked: return "Restocked";
        case ReturnStatus::Refunded:  return "Refunded";
        case ReturnStatus::Closed:    return "Closed";
    }
    return "Unknown";
}

static std::string shipmethod_str(ShippingMethod m) {
    return m == ShippingMethod::Express ? "Express" : "Standard";
}

static ShippingMethod shipmethod_from(const std::string& s) {
    return s == "Express" ? ShippingMethod::Express : ShippingMethod::Standard;
}

static std::string shipstatus_str(ShippingStatus s) {
    switch (s) {
        case ShippingStatus::Preparing:  return "Preparing";
        case ShippingStatus::InTransit:  return "InTransit";
        case ShippingStatus::Delivered:  return "Delivered";
        case ShippingStatus::Failed:     return "Failed";
    }
    return "Unknown";
}

// ── Domain serializers ────────────────────────────────────────────────────────

static std::string ser_product(
    const Product& p,
    const std::vector<std::string>& images = {}) {
    std::vector<std::string> vars;
    for (const auto& v : p.variants()) {
        vars.push_back(j_obj({
            {"variant_id",   j_str(v.id.value)},
            {"sku",          j_str(v.sku)},
            {"size",         j_str(v.size.value)},
            {"color",        j_str(v.color.value)},
            {"price_minor",  j_num(v.price.minor_units())},
            {"active",       j_bool(v.active)}
        }));
    }
    std::vector<std::string> image_items;
    for (const auto& image : images) {
        image_items.push_back(j_str(image));
    }
    return j_obj({
        {"product_id",   j_str(p.id().value)},
        {"name",         j_str(p.name())},
        {"category",     j_str(cat_str(p.category()))},
        {"description",  j_str(p.description())},
        {"collection",   j_str(p.collection())},
        {"status",       j_str(pstatus_str(p.status()))},
        {"images",       j_arr(image_items)},
        {"variants",     j_arr(vars)}
    });
}

static std::string ser_address(const ShippingAddress& address) {
    return j_obj({
        {"recipient_name", j_str(address.recipient_name)},
        {"phone",          j_str(address.phone)},
        {"line1",          j_str(address.line1)},
        {"line2",          j_str(address.line2)},
        {"ward",           j_str(address.ward)},
        {"district",       j_str(address.district)},
        {"city",           j_str(address.city)},
        {"country",        j_str(address.country)}
    });
}

static std::string ser_customer(const fashion_store::domain::customer::Customer& customer) {
    return j_obj({
        {"customer_id", j_str(customer.id().value)},
        {"account_id",  j_str(customer.account_id().value)},
        {"full_name",   j_str(customer.full_name())},
        {"phone",       j_str(customer.phone())},
        {"city",        j_str(customer.default_shipping_address().city)},
        {"address",     ser_address(customer.default_shipping_address())}
    });
}

static std::string ser_order(const Order& o) {
    std::vector<std::string> items;
    for (const auto& it : o.items()) {
        items.push_back(j_obj({
            {"order_item_id",    j_str(it.id.value)},
            {"product_id",       j_str(it.product_id.value)},
            {"variant_id",       j_str(it.variant_id.value)},
            {"product_name",     j_str(it.product_name)},
            {"sku",              j_str(it.sku)},
            {"size",             j_str(it.size.value)},
            {"color",            j_str(it.color.value)},
            {"unit_price_minor", j_num(it.unit_price.minor_units())},
            {"quantity",         j_num(it.quantity)},
            {"line_total_minor", j_num(it.line_total().minor_units())}
        }));
    }
    auto vc = o.voucher_code().has_value() ? j_str(*o.voucher_code()) : j_null();
    return j_obj({
        {"order_id",        j_str(o.id().value)},
        {"customer_id",     j_str(o.customer_id().value)},
        {"status",          j_str(ostatus_str(o.status()))},
        {"payment_status",  j_str(paystatus_str(o.payment_status()))},
        {"payment_method",  j_str(paymethod_str(o.payment_method()))},
        {"items",           j_arr(items)},
        {"subtotal_minor",  j_num(o.subtotal().minor_units())},
        {"discount_minor",  j_num(o.discount_total().minor_units())},
        {"total_minor",     j_num(o.total().minor_units())},
        {"voucher_code",    vc}
    });
}

static std::string ser_return(const ReturnRequest& r) {
    return j_obj({
        {"return_id",     j_str(r.id().value)},
        {"order_id",      j_str(r.order_id().value)},
        {"order_item_id", j_str(r.order_item_id().value)},
        {"quantity",      j_num(r.quantity())},
        {"reason",        j_str(r.reason())},
        {"status",        j_str(retstat_str(r.status()))}
    });
}

static std::string ser_review(const fashion_store::domain::review::Review& rv) {
    auto vid = rv.variant_id().has_value() ? j_str(rv.variant_id()->value) : j_null();
    return j_obj({
        {"review_id",         j_str(rv.id().value)},
        {"customer_id",       j_str(rv.customer_id().value)},
        {"product_id",        j_str(rv.product_id().value)},
        {"variant_id",        vid},
        {"rating",            j_num(rv.rating())},
        {"comment",           j_str(rv.comment())},
        {"verified_purchase", j_bool(rv.verified_purchase())}
    });
}

static std::string ser_shipment(const fashion_store::domain::shipping::Shipment& s) {
    return j_obj({
        {"order_id",      j_str(s.order_id().value)},
        {"tracking_code", j_str(s.tracking_code())},
        {"method",        j_str(shipmethod_str(s.method()))},
        {"fee_minor",     j_num(s.fee().minor_units())},
        {"status",        j_str(shipstatus_str(s.status()))}
    });
}

// ── Server setup ──────────────────────────────────────────────────────────────

static std::string place_order_error_message(
    const fashion_store::application::order::PlaceOrderError& error) {
    switch (error) {
        case fashion_store::application::order::PlaceOrderError::CartNotFound:
            return "Cart not found";
        case fashion_store::application::order::PlaceOrderError::EmptyCart:
            return "Cart is empty";
        case fashion_store::application::order::PlaceOrderError::ProductVariantNotFound:
            return "Selected variant was not found in backend data";
        case fashion_store::application::order::PlaceOrderError::InventoryNotFound:
            return "Inventory record was not found";
        case fashion_store::application::order::PlaceOrderError::InsufficientStock:
            return "Insufficient stock for one or more cart items";
        case fashion_store::application::order::PlaceOrderError::VoucherNotFound:
            return "Voucher was not found";
        case fashion_store::application::order::PlaceOrderError::VoucherInvalid:
            return "Voucher is invalid for this checkout";
        case fashion_store::application::order::PlaceOrderError::OrderRuleViolation:
            return "Order state rule prevented checkout";
    }
    return "Checkout failed";
}

static std::string slugify(std::string value) {
    std::string slug;
    slug.reserve(value.size());
    bool previous_dash = false;
    for (unsigned char ch : value) {
        if (std::isalnum(ch)) {
            slug.push_back(static_cast<char>(std::tolower(ch)));
            previous_dash = false;
            continue;
        }
        if (!previous_dash) {
            slug.push_back('-');
            previous_dash = true;
        }
    }
    while (!slug.empty() && slug.front() == '-') {
        slug.erase(slug.begin());
    }
    while (!slug.empty() && slug.back() == '-') {
        slug.pop_back();
    }
    if (slug.empty()) {
        return "user";
    }
    return slug;
}

static void add_cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

static void json_ok(httplib::Response& res, const std::string& data) {
    add_cors(res);
    res.set_content(ok_json(data), "application/json");
}

static void json_err(httplib::Response& res, int status, const std::string& msg) {
    add_cors(res);
    res.status = status;
    res.set_content(err_json(msg), "application/json");
}

using namespace fashion_store::application;

inline void setup_server(
    httplib::Server& svr,
    identity::AuthApplicationService&              auth_svc,
    catalog::CatalogApplicationService&            catalog_svc,
    cart::CartApplicationService&                  cart_svc,
    domain::identity::IAccountRepository&          account_repo,
    domain::customer::ICustomerRepository&         customer_repo,
    order::OrderApplicationService&                order_svc,
    review::ReviewApplicationService&              review_svc,
    returns::ReturnsApplicationService&            returns_svc,
    staff::ReturnManagementService&                return_mgmt_svc,
    staff::StoreManagementService&                 store_mgmt_svc,
    payment::PaymentApplicationService&            payment_svc,
    shipping::ShippingApplicationService&          shipping_svc,
    report::ReportApplicationService&              report_svc,
    const std::filesystem::path&                   product_storefront_path)
{
    auto product_images_for = [&](const ProductId& product_id) {
        const auto metadata = load_product_storefront_metadata(product_storefront_path);
        const auto match = metadata.images_by_product.find(product_id.value);
        return match != metadata.images_by_product.end()
            ? match->second
            : std::vector<std::string>{};
    };

    auto save_product_images = [&](const ProductId& product_id, const std::string& image_urls_text) {
        auto metadata = load_product_storefront_metadata(product_storefront_path);
        const auto images = parse_image_urls_text(image_urls_text);
        if (images.empty()) {
            metadata.images_by_product.erase(product_id.value);
        } else {
            metadata.images_by_product[product_id.value] = images;
        }
        save_product_storefront_metadata(product_storefront_path, metadata);
    };

    auto ser_product_with_storefront = [&](const Product& product) {
        return ser_product(product, product_images_for(product.id()));
    };

    // CORS preflight
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        res.set_content("", "text/plain");
    });

    // ── Auth ──────────────────────────────────────────────────────────────────
    svr.Post("/api/sign-in", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        auto username = b.str("username");
        auto pw_hash  = b.str("password_hash", b.str("passwordHash"));
        auto result   = auth_svc.sign_in(username, pw_hash);
        if (!result) {
            json_err(res, 401, "Invalid credentials");
            return;
        }
        const auto& acc = result.value().account;
        std::string role;
        switch (acc.role()) {
            case domain::identity::Role::Customer: role = "Customer"; break;
            case domain::identity::Role::Staff:    role = "Staff";    break;
            case domain::identity::Role::Manager:  role = "Manager";  break;
            case domain::identity::Role::Admin:    role = "Admin";    break;
        }
        auto cust = customer_repo.find_by_account_id(acc.id());
        auto customer_id = cust.has_value() ? j_str(cust->id().value) : j_null();
        auto display_name = cust.has_value() ? j_str(cust->full_name()) : j_str(acc.username());
        auto customer_json = cust.has_value() ? ser_customer(*cust) : j_null();
        json_ok(res, j_obj({
            {"account_id",   j_str(acc.id().value)},
            {"username",     j_str(acc.username())},
            {"role",         j_str(role)},
            {"customer_id",  customer_id},
            {"display_name", display_name},
            {"customer",     customer_json}
        }));
    });

    svr.Post("/api/register", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        const auto username = b.str("username");
        const auto password_hash = b.str("password_hash", b.str("passwordHash"));
        const auto full_name = b.str("full_name", b.str("fullName"));
        const auto phone = b.str("phone");
        if (username.empty() || password_hash.empty() || full_name.empty() || phone.empty()) {
            json_err(res, 400, "Username, password, full name, and phone are required");
            return;
        }
        if (account_repo.find_by_username(username).has_value()) {
            json_err(res, 409, "Username already exists");
            return;
        }

        const auto slug = slugify(username);
        std::string account_id = "account-" + slug;
        std::string customer_id = "customer-" + slug;
        int suffix = 1;
        while (account_repo.find_by_id(AccountId{account_id}).has_value() ||
               customer_repo.find_by_id(CustomerId{customer_id}).has_value()) {
            ++suffix;
            account_id = "account-" + slug + "-" + std::to_string(suffix);
            customer_id = "customer-" + slug + "-" + std::to_string(suffix);
        }

        const ShippingAddress address{
            b.str("recipient_name", full_name),
            phone,
            b.str("line1", "12 Nguyen Hue"),
            b.str("line2"),
            b.str("ward", "Ben Nghe"),
            b.str("district", "District 1"),
            b.str("city", "Ho Chi Minh City"),
            b.str("country", "Vietnam")
        };

        const domain::identity::Account account(
            AccountId{account_id},
            username,
            password_hash,
            domain::identity::Role::Customer,
            domain::identity::AccountStatus::Active);
        const fashion_store::domain::customer::Customer customer(
            CustomerId{customer_id},
            AccountId{account_id},
            full_name,
            phone,
            address);

        account_repo.save(account);
        customer_repo.save(customer);

        json_ok(res, j_obj({
            {"account_id",   j_str(account.id().value)},
            {"username",     j_str(account.username())},
            {"role",         j_str("Customer")},
            {"customer_id",  j_str(customer.id().value)},
            {"display_name", j_str(customer.full_name())},
            {"customer",     ser_customer(customer)}
        }));
    });

    // ── Catalog ───────────────────────────────────────────────────────────────
    svr.Get("/api/products", [&](const httplib::Request& req, httplib::Response& res) {
        catalog::ProductSearchQuery q;
        if (req.has_param("q"))        q.keyword  = req.get_param_value("q");
        if (req.has_param("category")) q.category = cat_from(req.get_param_value("category"));
        if (req.has_param("sort")) {
            auto s = req.get_param_value("sort");
            if (s == "price_asc")  q.sort_mode = catalog::ProductSortMode::PriceAsc;
            if (s == "price_desc") q.sort_mode = catalog::ProductSortMode::PriceDesc;
        }
        auto products = catalog_svc.search_products(q);
        std::vector<std::string> arr;
        for (const auto& p : products) arr.push_back(ser_product_with_storefront(p));
        json_ok(res, j_arr(arr));
    });

    svr.Get("/api/products/:id", [&](const httplib::Request& req, httplib::Response& res) {
        auto p = catalog_svc.find_product(ProductId{req.path_params.at("id")});
        if (!p) { json_err(res, 404, "Product not found"); return; }
        json_ok(res, ser_product_with_storefront(*p));
    });

    svr.Get("/api/variants/:id", [&](const httplib::Request& req, httplib::Response& res) {
        auto v = catalog_svc.find_variant(VariantId{req.path_params.at("id")});
        if (!v) { json_err(res, 404, "Variant not found"); return; }
        json_ok(res, j_obj({
            {"variant_id",  j_str(v->variant.id.value)},
            {"product_id",  j_str(v->product_id.value)},
            {"product_name",j_str(v->product_name)},
            {"sku",         j_str(v->variant.sku)},
            {"size",        j_str(v->variant.size.value)},
            {"color",       j_str(v->variant.color.value)},
            {"price_minor", j_num(v->variant.price.minor_units())},
            {"active",      j_bool(v->variant.active)}
        }));
    });

    // ── Cart ──────────────────────────────────────────────────────────────────
    svr.Get("/api/customers/:id", [&](const httplib::Request& req, httplib::Response& res) {
        auto customer = customer_repo.find_by_id(CustomerId{req.path_params.at("id")});
        if (!customer.has_value()) {
            json_err(res, 404, "Customer not found");
            return;
        }
        json_ok(res, ser_customer(*customer));
    });



    svr.Post("/api/cart/add", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        auto result = cart_svc.add_item(
            CartId{b.str("cart_id")},
            CustomerId{b.str("customer_id")},
            VariantId{b.str("variant_id")},
            static_cast<int>(b.num("quantity", 1)));
        if (!result) { json_err(res, 400, "Cart add failed"); return; }
        json_ok(res, j_obj({{"cart_id", j_str(result.value().id().value)},
                             {"item_count", j_num(result.value().items().size())}}));
    });

    svr.Post("/api/cart/quantity", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        auto result = cart_svc.change_quantity(
            CartId{b.str("cart_id")},
            VariantId{b.str("variant_id")},
            static_cast<int>(b.num("quantity")));
        if (!result) { json_err(res, 400, "Cart update failed"); return; }
        json_ok(res, j_obj({{"cart_id", j_str(result.value().id().value)}}));
    });

    svr.Post("/api/cart/remove", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        auto result = cart_svc.remove_item(
            CartId{b.str("cart_id")},
            VariantId{b.str("variant_id")});
        if (!result) { json_err(res, 400, "Cart remove failed"); return; }
        json_ok(res, j_obj({{"cart_id", j_str(result.value().id().value)}}));
    });

    // ── Checkout ──────────────────────────────────────────────────────────────
    svr.Post("/api/checkout/preview", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        std::optional<std::string> vc;
        if (b.has("voucher_code") && !b.str("voucher_code").empty())
            vc = b.str("voucher_code");
        auto result = order_svc.preview_checkout(CartId{b.str("cart_id")}, vc);
        if (!result) { json_err(res, 400, place_order_error_message(result.error())); return; }
        const auto& pv = result.value();
        json_ok(res, j_obj({
            {"item_count",      j_num(pv.item_count)},
            {"subtotal_minor",  j_num(pv.subtotal.minor_units())},
            {"discount_minor",  j_num(pv.discount.minor_units())},
            {"total_minor",     j_num(pv.total.minor_units())},
            {"voucher_applied", j_bool(pv.voucher_applied)}
        }));
    });

    svr.Post("/api/checkout", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        std::optional<std::string> vc;
        if (b.has("voucher_code") && !b.str("voucher_code").empty())
            vc = b.str("voucher_code");
        ShippingAddress addr{
            b.str("recipient_name"),
            b.str("phone"),
            b.str("line1"),
            b.str("line2"),
            b.str("ward"),
            b.str("district"),
            b.str("city", "Ho Chi Minh City"),
            b.str("country", "Vietnam")
        };
        order::PlaceOrderCommand cmd{
            OrderId{b.str("order_id")},
            CartId{b.str("cart_id")},
            CustomerId{b.str("customer_id")},
            addr,
            paymethod_from(b.str("method", "Cash")),
            vc
        };
        auto result = order_svc.place_order(cmd);
        if (!result) { json_err(res, 400, place_order_error_message(result.error())); return; }
        const auto& r = result.value();
        json_ok(res, j_obj({
            {"order",          ser_order(r.order)},
            {"subtotal_minor", j_num(r.subtotal.minor_units())},
            {"discount_minor", j_num(r.discount.minor_units())},
            {"total_minor",    j_num(r.total.minor_units())}
        }));
    });

    // ── Orders ────────────────────────────────────────────────────────────────
    svr.Get("/api/orders/:id", [&](const httplib::Request& req, httplib::Response& res) {
        auto result = order_svc.get_order(OrderId{req.path_params.at("id")});
        if (!result) { json_err(res, 404, "Order not found"); return; }
        json_ok(res, ser_order(result.value()));
    });

    svr.Get("/api/customers/:id/orders", [&](const httplib::Request& req, httplib::Response& res) {
        auto orders = order_svc.get_customer_orders(CustomerId{req.path_params.at("id")});
        std::vector<std::string> arr;
        for (const auto& o : orders) arr.push_back(ser_order(o));
        json_ok(res, j_arr(arr));
    });

    svr.Post("/api/orders/:id/pay", [&](const httplib::Request& req, httplib::Response& res) {
        auto result = order_svc.mark_order_paid(OrderId{req.path_params.at("id")});
        if (!result) { json_err(res, 400, "Mark paid failed"); return; }
        json_ok(res, ser_order(result.value()));
    });

    svr.Post("/api/orders/:id/advance", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        auto result = order_svc.advance_order_status(
            OrderId{req.path_params.at("id")},
            ostatus_from(b.str("status")));
        if (!result) { json_err(res, 400, "Advance status failed"); return; }
        json_ok(res, ser_order(result.value()));
    });

    svr.Post("/api/orders/:id/cancel", [&](const httplib::Request& req, httplib::Response& res) {
        auto result = order_svc.cancel_order(OrderId{req.path_params.at("id")});
        if (!result) { json_err(res, 400, "Cancel failed"); return; }
        json_ok(res, ser_order(result.value()));
    });

    // ── Reviews ───────────────────────────────────────────────────────────────
    svr.Post("/api/reviews", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        std::optional<VariantId> vid;
        if (b.has("variant_id") && !b.str("variant_id").empty())
            vid = VariantId{b.str("variant_id")};
        auto result = review_svc.create_review(
            OrderId{b.str("order_id")},
            ReviewId{b.str("review_id")},
            CustomerId{b.str("customer_id")},
            ProductId{b.str("product_id")},
            vid,
            static_cast<int>(b.num("rating", 5)),
            b.str("comment"));
        if (!result) { json_err(res, 400, "Review failed"); return; }
        json_ok(res, ser_review(result.value()));
    });

    svr.Get("/api/products/:id/reviews", [&](const httplib::Request& req, httplib::Response& res) {
        auto reviews = review_svc.get_product_reviews(ProductId{req.path_params.at("id")});
        std::vector<std::string> arr;
        for (const auto& rv : reviews) arr.push_back(ser_review(rv));
        json_ok(res, j_arr(arr));
    });

    svr.Get("/api/customers/:id/reviews", [&](const httplib::Request& req, httplib::Response& res) {
        auto reviews = review_svc.get_customer_reviews(CustomerId{req.path_params.at("id")});
        std::vector<std::string> arr;
        for (const auto& rv : reviews) arr.push_back(ser_review(rv));
        json_ok(res, j_arr(arr));
    });

    // ── Returns (customer) ────────────────────────────────────────────────────
    svr.Post("/api/returns", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        auto result = returns_svc.create_return_request(
            OrderId{b.str("order_id")},
            ReturnId{b.str("return_id")},
            OrderItemId{b.str("order_item_id")},
            static_cast<int>(b.num("quantity", 1)),
            b.str("reason"));
        if (!result) { json_err(res, 400, "Return request failed"); return; }
        json_ok(res, ser_return(result.value()));
    });

    svr.Get("/api/orders/:id/returns", [&](const httplib::Request& req, httplib::Response& res) {
        auto rets = returns_svc.get_order_returns(OrderId{req.path_params.at("id")});
        std::vector<std::string> arr;
        for (const auto& r : rets) arr.push_back(ser_return(r));
        json_ok(res, j_arr(arr));
    });

    // ── Returns (staff) ───────────────────────────────────────────────────────
    auto make_return_action = [&](const std::string& action) {
        return [&, action](const httplib::Request& req, httplib::Response& res) {
            ReturnId rid{req.path_params.at("id")};
            std::optional<ReturnRequest> updated;
            if (action == "approve") {
                auto r = return_mgmt_svc.approve_return(rid);
                if (!r) { json_err(res, 400, "Approve failed"); return; }
                json_ok(res, ser_return(r.value()));
            } else if (action == "reject") {
                auto r = return_mgmt_svc.reject_return(rid);
                if (!r) { json_err(res, 400, "Reject failed"); return; }
                json_ok(res, ser_return(r.value()));
            } else if (action == "restock") {
                auto r = return_mgmt_svc.restock_return(rid);
                if (!r) { json_err(res, 400, "Restock failed"); return; }
                json_ok(res, ser_return(r.value()));
            } else if (action == "refund") {
                auto r = return_mgmt_svc.refund_return(rid);
                if (!r) { json_err(res, 400, "Refund failed"); return; }
                json_ok(res, ser_return(r.value()));
            } else {
                auto r = return_mgmt_svc.close_return(rid);
                if (!r) { json_err(res, 400, "Close failed"); return; }
                json_ok(res, ser_return(r.value()));
            }
        };
    };

    svr.Post("/api/returns/:id/approve",  make_return_action("approve"));
    svr.Post("/api/returns/:id/reject",   make_return_action("reject"));
    svr.Post("/api/returns/:id/restock",  make_return_action("restock"));
    svr.Post("/api/returns/:id/refund",   make_return_action("refund"));
    svr.Post("/api/returns/:id/close",    make_return_action("close"));

    // ── Staff ─────────────────────────────────────────────────────────────────
    svr.Post("/api/staff/products", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        staff::ProductDraft draft{
            ProductId{b.str("product_id")},
            b.str("name"),
            cat_from(b.str("category")),
            b.str("description"),
            b.str("collection"),
            pstatus_from(b.str("status", "Active"))
        };
        auto result = store_mgmt_svc.create_product(draft);
        if (!result) { json_err(res, 400, "Create product failed"); return; }
        save_product_images(draft.id, b.str("image_urls_text"));
        json_ok(res, ser_product_with_storefront(result.value()));
    });

    svr.Post("/api/staff/products/:id/images", [&](const httplib::Request& req, httplib::Response& res) {
        const auto product_id = ProductId{req.path_params.at("id")};
        auto product = catalog_svc.find_product(product_id);
        if (!product) { json_err(res, 404, "Product not found"); return; }
        auto b = parse_body(req.body);
        save_product_images(product_id, b.str("image_urls_text"));
        product = catalog_svc.find_product(product_id);
        if (!product) { json_err(res, 404, "Product not found"); return; }
        json_ok(res, ser_product_with_storefront(*product));
    });

    svr.Post("/api/staff/products/:id/status", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        auto result = store_mgmt_svc.update_product_status(
            ProductId{req.path_params.at("id")},
            pstatus_from(b.str("status")));
        if (!result) { json_err(res, 400, "Update status failed"); return; }
        json_ok(res, ser_product_with_storefront(result.value()));
    });

    svr.Post("/api/staff/products/:id/variants", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        ProductVariantDraft vd{
            VariantId{b.str("variant_id")},
            b.str("sku"),
            Size{b.str("size")},
            Color{b.str("color")},
            Money::from_minor(b.num("price_minor"))
        };
        auto result = store_mgmt_svc.add_product_variant(
            ProductId{req.path_params.at("id")}, vd);
        if (!result) { json_err(res, 400, "Add variant failed"); return; }
        json_ok(res, ser_product_with_storefront(result.value()));
    });

    svr.Post("/api/staff/inventory", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        auto result = store_mgmt_svc.set_inventory(
            VariantId{b.str("variant_id")},
            static_cast<int>(b.num("on_hand")),
            static_cast<int>(b.num("reserved", 0)));
        if (!result) { json_err(res, 400, "Set inventory failed"); return; }
        const auto& inv = result.value();
        json_ok(res, j_obj({
            {"variant_id", j_str(inv.variant_id().value)},
            {"on_hand",    j_num(inv.on_hand())},
            {"reserved",   j_num(inv.reserved())},
            {"available",  j_num(inv.available())}
        }));
    });

    svr.Post("/api/staff/inventory/:id/restock", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        auto result = store_mgmt_svc.restock_inventory(
            VariantId{req.path_params.at("id")},
            static_cast<int>(b.num("quantity")));
        if (!result) { json_err(res, 400, "Restock failed"); return; }
        const auto& inv = result.value();
        json_ok(res, j_obj({
            {"variant_id", j_str(inv.variant_id().value)},
            {"on_hand",    j_num(inv.on_hand())},
            {"reserved",   j_num(inv.reserved())},
            {"available",  j_num(inv.available())}
        }));
    });

    svr.Post("/api/staff/vouchers", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        auto now = std::chrono::system_clock::now();
        domain::pricing::Voucher v(
            b.str("code"),
            domain::pricing::DiscountType::Percentage,
            static_cast<int>(b.num("rate_percent", 10)),
            Money::from_minor(b.num("min_order_minor", 0)),
            Money::from_minor(b.num("max_discount_minor", 0)),
            now,
            now + std::chrono::hours(24 * 30),
            static_cast<int>(b.num("max_uses", 100)),
            true);
        store_mgmt_svc.save_voucher(v);
        json_ok(res, j_obj({{"code", j_str(b.str("code"))}, {"saved", j_bool(true)}}));
    });

    svr.Get("/api/staff/orders", [&](const httplib::Request&, httplib::Response& res) {
        auto orders = store_mgmt_svc.list_orders();
        std::vector<std::string> arr;
        for (const auto& o : orders) arr.push_back(ser_order(o));
        json_ok(res, j_arr(arr));
    });

    svr.Post("/api/staff/orders/:id/advance", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        auto result = store_mgmt_svc.advance_order_status(
            OrderId{req.path_params.at("id")},
            ostatus_from(b.str("status")));
        if (!result) { json_err(res, 400, "Advance failed"); return; }
        json_ok(res, ser_order(result.value()));
    });

    svr.Post("/api/staff/orders/:id/cancel", [&](const httplib::Request& req, httplib::Response& res) {
        auto result = store_mgmt_svc.cancel_order(OrderId{req.path_params.at("id")});
        if (!result) { json_err(res, 400, "Cancel failed"); return; }
        json_ok(res, ser_order(result.value()));
    });

    // ── Payment ───────────────────────────────────────────────────────────────
    svr.Post("/api/payment/authorize", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        auto order = order_svc.get_order(OrderId{b.str("order_id")});
        if (!order) { json_err(res, 404, "Order not found"); return; }
        auto result = payment_svc.authorize_order_payment(order.value());
        if (!result) { json_err(res, 400, "Payment failed"); return; }
        const auto& r = result.value();
        json_ok(res, j_obj({
            {"order_id",     j_str(r.order_id.value)},
            {"method",       j_str(paymethod_str(r.method))},
            {"amount_minor", j_num(r.amount.minor_units())},
            {"reference",    j_str(r.reference)}
        }));
    });

    // ── Shipping ──────────────────────────────────────────────────────────────
    svr.Get("/api/shipping/quote", [&](const httplib::Request& req, httplib::Response& res) {
        auto method = shipmethod_from(req.has_param("method") ? req.get_param_value("method") : "Standard");
        auto quote  = shipping_svc.quote(method);
        json_ok(res, j_obj({
            {"method",          j_str(shipmethod_str(quote.method))},
            {"fee_minor",       j_num(quote.fee.minor_units())},
            {"estimated_days",  j_num(quote.estimated_days)}
        }));
    });

    svr.Post("/api/shipping/shipments", [&](const httplib::Request& req, httplib::Response& res) {
        auto b = parse_body(req.body);
        auto order = order_svc.get_order(OrderId{b.str("order_id")});
        if (!order) { json_err(res, 404, "Order not found"); return; }
        auto result = shipping_svc.create_shipment(
            order.value(),
            shipmethod_from(b.str("method", "Standard")));
        if (!result) { json_err(res, 400, "Shipment failed"); return; }
        json_ok(res, ser_shipment(result.value()));
    });

    // ── Reports ───────────────────────────────────────────────────────────────
    svr.Get("/api/reports/revenue", [&](const httplib::Request&, httplib::Response& res) {
        auto rpt = report_svc.build_revenue_report();
        json_ok(res, j_obj({
            {"order_count",        j_num(rpt.order_count)},
            {"item_count",         j_num(rpt.item_count)},
            {"total_revenue_minor",j_num(rpt.total_revenue.minor_units())}
        }));
    });

    svr.Get("/api/reports/best-selling", [&](const httplib::Request&, httplib::Response& res) {
        auto rows = report_svc.build_best_selling_products();
        std::vector<std::string> arr;
        for (const auto& r : rows) {
            arr.push_back(j_obj({
                {"product_id",        j_str(r.product_id.value)},
                {"product_name",      j_str(r.product_name)},
                {"quantity_sold",     j_num(r.quantity_sold)},
                {"gross_sales_minor", j_num(r.gross_sales.minor_units())}
            }));
        }
        json_ok(res, j_arr(arr));
    });

    svr.Get("/api/reports/low-stock", [&](const httplib::Request& req, httplib::Response& res) {
        int threshold = req.has_param("threshold")
            ? std::stoi(req.get_param_value("threshold")) : 5;
        auto rows = report_svc.build_low_stock_report(threshold);
        std::vector<std::string> arr;
        for (const auto& r : rows) {
            arr.push_back(j_obj({
                {"variant_id", j_str(r.variant_id.value)},
                {"on_hand",    j_num(r.on_hand)},
                {"reserved",   j_num(r.reserved)},
                {"available",  j_num(r.available)}
            }));
        }
        json_ok(res, j_arr(arr));
    });
}

}  // namespace fashion_store::server
