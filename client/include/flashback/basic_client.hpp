#pragma once

#include <string>

namespace flashback
{
class basic_client
{
public:
    virtual ~basic_client() = default;
    virtual bool signup(std::string name, std::string email, std::string password) = 0;
};
} // flashback
