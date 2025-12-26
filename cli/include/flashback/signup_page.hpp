#pragma once

#include <flashback/client.hpp>
#include <flashback/page.hpp>

namespace flashback
{
class window_manager;

class signup_page final: public page
{
public:
    explicit signup_page(std::shared_ptr<client> client, std::weak_ptr<window_manager> window);
    ~signup_page() override = default;
    std::pair<ftxui::Component, std::function<ftxui::Element()>> prepare_components() override;

private:
    bool verify() const;
    void submit();
    void verify_submit();

private:
    std::shared_ptr<client> m_client;
    std::weak_ptr<window_manager> m_window_manager;
    std::string m_name;
    std::string m_email;
    std::string m_password;
    std::string m_verify_password;
};
} // flashback
