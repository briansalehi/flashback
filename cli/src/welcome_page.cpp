#include <algorithm>
#include <memory>
#include <utility>
#include <iostream>
#include <format>
#include <flashback/welcome_page.hpp>

using namespace flashback;

welcome_page::welcome_page(std::shared_ptr<client> client)
    : m_client{client}
{
    // bool credentials_valid{server->check_user_credentials()};
    // std::set<roadmap> const roadmaps{m_server->roadmaps(2)};
    // m_page = std::make_shared<welcome_page>(m_client);

    display();
}

void welcome_page::display()
{
    content(ftxui::hbox(ftxui::vbox(m_elements, ftxui::flex), ftxui::flex));
    page::display();
}
