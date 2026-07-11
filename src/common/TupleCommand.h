#pragma once

#include <any>
#include <functional>
#include <tuple>
#include <utility>

#include "common/ICommandBase.h"

template<typename... Args>
class TupleCommand final : public ICommandBase {
public:
    using Parameters = std::tuple<Args...>;
    using Handler = std::function<void(const Parameters&)>;

    explicit TupleCommand(Handler handler)
        : handler_(std::move(handler))
    {
    }

    void execute(const std::any& parameters = std::any{}) override
    {
        if constexpr (sizeof...(Args) == 0) {
            if (!parameters.has_value()) {
                handler_(Parameters{});
                return;
            }
        }

        handler_(std::any_cast<const Parameters&>(parameters));
    }

private:
    Handler handler_;
};
