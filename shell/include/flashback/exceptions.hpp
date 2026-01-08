#pragma once

#include <string>
#include <exception>

namespace flashback
{
class descriptive_option final: public std::exception
{
public:
    explicit descriptive_option(std::string info): description{std::move(info)}
    {
    }

    [[nodiscard]] char const* what() const noexcept override
    {
        return description.c_str();
    }
private:
    std::string description;
};
} // flashback