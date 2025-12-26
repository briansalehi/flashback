#pragma once

#include <memory>
#include <flashback/page.hpp>
#include <flashback/client.hpp>

namespace flashback
{
class window_manager;

class signin_page final: public page
{
public:
    explicit signin_page(std::shared_ptr<client> client, std::shared_ptr<window_manager> window);
    std::pair<ftxui::Component, std::function<ftxui::Element()>> prepare_components() override;

private:
    bool verify() const;
    void submit();
    void verify_submit();

private:
    std::shared_ptr<client> m_client;
    std::shared_ptr<window_manager> m_window_manager;
    std::string m_email;
    std::string m_password;
    std::string m_device;
};
} // flashback
