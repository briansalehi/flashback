#pragma once

#include <string>
#include <types.pb.h>

namespace flashback
{
class basic_client
{
public:
    virtual ~basic_client() = default;
    virtual bool signup(std::string name, std::string email, std::string password) = 0;
    virtual bool signin(std::string email, std::string password) = 0;
    virtual bool needs_to_signup() const = 0;
    virtual bool needs_to_signin() const = 0;
    virtual std::vector<Roadmap> get_roadmaps() const = 0;
};
} // flashback
