#include <flashback/signup_page.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>

using namespace flashback;

signup_page::signup_page(std::shared_ptr<client> client)
    : m_client{client}
{
    // ftxui::Component signin_button{};
    page::display(ftxui::text("register"));
}
