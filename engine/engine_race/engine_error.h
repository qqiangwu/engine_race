#ifndef ENGINE_RACE_ENGINE_RACE_ERROR_H_
#define ENGINE_RACE_ENGINE_RACE_ERROR_H_

#include <stdexcept>

namespace zero_switch {

class Task_canceled : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

}

#endif
