#pragma once

#include <string>
#include <utility>
#include <exception>

namespace flashback
{
class client_exception final : public std::exception
{
    std::string m_reason;

public:
    client_exception(std::string reason)
        : m_reason{std::move(reason)}
    {
    }

    [[nodiscard]] const char* what() const noexcept override
    {
        return m_reason.c_str();
    }
};
} // flashback
