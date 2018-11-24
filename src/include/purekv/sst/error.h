#pragma once

#include <stdexcept>

namespace zero_switch {

class Data_corruption_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

}
