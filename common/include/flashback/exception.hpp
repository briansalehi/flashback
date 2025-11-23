#pragma once

#include <string>
#include <utility>
#include <exception>

namespace flashback
{
class client_exception final : public std::exception
{
    std::string m_reason;
    std::string m_code;

public:
    client_exception(std::string reason)
        : m_reason{std::move(reason)}
    {
    }

    [[nodiscard]] const char* what() const noexcept override
    {
        return m_reason.c_str();
    }

    [[nodiscard]] std::string code() const noexcept
    {
        return m_code;
    }
};
} // flashback
