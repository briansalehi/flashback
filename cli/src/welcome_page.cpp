#include <memory>
#include <flashback/welcome_page.hpp>
#include <grpc/grpc.h>

using namespace flashback;

welcome_page::welcome_page(std::shared_ptr<client> client)
    : m_client{client}
{
    try
    {
        if (!m_client->user_is_defined())
        {
            auto user{std::make_unique<flashback::User>()};
            user->set_email("briansalehi@proton.me");
            user->set_hash("****");
            user->set_id(2);
            user->set_name("Brian Salehi");
            user->set_password("1234");
            user->set_verified(false);
            user->set_state(User_State_active);
            user->set_device("fedora");

            m_client->set_user(std::move(user));
            std::shared_ptr<SignInResponse> signin_response{m_client->signin()};
        }
        // std::shared_ptr<SignUpResponse> signup_response{m_client->signup()};
        // display();
    }
    catch (std::runtime_error const& exp)
    {
        std::cerr << exp.what() << std::endl;
    }
}

void welcome_page::display()
{
    content(ftxui::hbox(ftxui::vbox(m_elements, ftxui::flex), ftxui::flex));
    page::display();
}
