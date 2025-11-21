#pragma once

#include <memory>
#include <flashback/client.hpp>
#include <flashback/page.hpp>

namespace flashback
{
class reset_password_page : public page
{
public:
    explicit reset_password_page(std::shared_ptr<client> client);
    ~reset_password_page() override = default;

private:
    bool verify();
    void submit();
    void verify_submit();

    std::shared_ptr<client> m_client;
    std::string m_name;
    std::string m_email;
    std::string m_password;
    std::string m_verify_password;
};
} // flashback
