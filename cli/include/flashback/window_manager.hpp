#pragma once

#include <memory>
#include <flashback/page.hpp>
#include <flashback/client.hpp>

namespace flashback
{
class window_manager
{
public:
    explicit window_manager(std::shared_ptr<client> client, std::shared_ptr<User> user);
    ~window_manager() = default;

private:
    std::shared_ptr<client> m_client;
    std::shared_ptr<page> m_page;
};
} // flashback
