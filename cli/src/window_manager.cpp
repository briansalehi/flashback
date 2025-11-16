#include <flashback/window_manager.hpp>
#include <flashback/welcome_page.hpp>
#include <flashback/signup_page.hpp>
// #include <flashback/signin_page.hpp>
#include <flashback/roadmap_page.hpp>

using namespace flashback;

window_manager::window_manager(std::shared_ptr<client> client)
    : m_client{client}
    , m_page{std::make_shared<welcome_page>()}
    , m_window_lock{std::make_unique<std::mutex>()}
    , m_window_runner{std::make_unique<std::jthread>(&window_manager::display, this)}
{
    try
    {
        auto user{std::make_unique<flashback::User>()};
        user->set_email("briansalehi@proton.me");
        user->set_hash("****");
        user->set_id(2);
        user->set_name("Brian Salehi");
        user->set_password("abcdef");
        user->set_verified(false);
        user->set_state(User_State_active);
        user->set_device("fedora");
        m_client->set_user(std::move(user));

        auto new_page{std::make_shared<roadmap_page>(m_client)};
        m_page->close();
        m_window_runner->join();
        m_page = new_page;
        m_window_runner = std::make_unique<std::jthread>(&window_manager::display, this);
        m_window_runner->join();

        /*
        if (!m_client->user_is_defined())
        {
            m_page = std::make_shared<signup_page>(m_client);
        }
        else if (!m_client->session_is_valid())
        {
            // m_page = std::make_shared<signin_page>(m_client);
        }
        else
        {
            m_page = std::make_shared<roadmap_page>(m_client);
        }
        */
    }
    catch (std::runtime_error const& exp)
    {
        std::cerr << exp.what() << std::endl;
    }
}

void window_manager::display()
{
    std::lock_guard<std::mutex> lock{*m_window_lock};
    m_page->render();
}
