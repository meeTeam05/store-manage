#pragma once

#include <optional>
#include <string_view>

#include "domain/pricing/voucher.hpp"

namespace fashion_store::domain::pricing {

class IVoucherRepository {
public:
    virtual ~IVoucherRepository() = default;

    virtual std::optional<Voucher> find_by_code(std::string_view code) const = 0;
    virtual void save(const Voucher& voucher) = 0;
};

}  // namespace fashion_store::domain::pricing
