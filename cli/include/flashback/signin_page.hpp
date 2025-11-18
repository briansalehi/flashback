#pragma once

#include <memory>
#include <flashback/page.hpp>
#include <flashback/client.hpp>

namespace flashback
{
class signin_page final: public page
{
public:
    explicit signin_page(std::shared_ptr<client> client);

private:
    bool verify();
    void submit();
    void verify_submit();

    std::shared_ptr<client> m_client;
    std::string m_email;
    std::string m_password;
};
} // flashback