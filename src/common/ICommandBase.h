#pragma once

#include <any>

class ICommandBase {
public:
    virtual ~ICommandBase() = default;

    virtual void execute(const std::any& parameters = std::any{}) = 0;
};
