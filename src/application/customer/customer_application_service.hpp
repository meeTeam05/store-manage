#pragma once

#include <string>

namespace fashion_store::application::customer {

struct CustomerProfileView {
    std::string full_name;
    std::string phone;
    std::string city;
};

class CustomerApplicationService {
public:
    CustomerProfileView build_placeholder_profile() const {
        return CustomerProfileView{
            "Maison Aureline Client",
            "0900000000",
            "Ho Chi Minh City"
        };
    }
};

}  // namespace fashion_store::application::customer
