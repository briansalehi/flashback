#pragma once

#include <memory>
#include <flashback/client.hpp>
#include <flashback/page.hpp>

namespace flashback
{
class signup_page : public page
{
public:
    explicit signup_page(std::shared_ptr<client> client);
    ~signup_page() override = default;

private:
    std::shared_ptr<client> m_client;
};
} // flashback
