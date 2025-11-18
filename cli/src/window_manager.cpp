#include <flashback/window_manager.hpp>
#include <flashback/welcome_page.hpp>
#include <flashback/signup_page.hpp>
#include <flashback/signin_page.hpp>
#include <flashback/roadmap_page.hpp>
#include <chrono>

using namespace flashback;

window_manager::window_manager(std::shared_ptr<client> client)
    : m_client{client}
    , m_page{nullptr}
    , m_next_page{nullptr}
    , m_window_lock{std::make_unique<std::mutex>()}
    , m_window_runner{nullptr}
{
    try
    {
        auto user{std::make_unique<flashback::User>()};
        user->set_email("briansalehi@proton.me");
        user->set_password("abcdef");
        user->set_device("fedora");
        // m_client->user(std::move(user));
        // m_client->token("dummy-token");
        // swap_page(std::make_shared<welcome_page>());

        if (!m_client->user_is_defined())
        {
            swap_page(std::make_shared<signup_page>(m_client));
            m_window_runner->join();
        }

        if (!m_client->session_is_valid())
        {
            swap_page(std::make_shared<signin_page>(m_client));
            m_window_runner->join();
            if (!m_client->session_is_valid())
            {
                throw std::runtime_error("Client: sign in failed");
            }
        }

        swap_page(std::make_shared<roadmap_page>(m_client));
        m_window_runner->join();
    }
    catch (std::runtime_error const& exp)
    {
        m_page->close();
        if (m_window_runner->joinable())
            m_window_runner->join();
        m_window_runner.reset();
        std::cerr << exp.what() << std::endl;
    }
}

void window_manager::display()
{
    if (m_page != nullptr)
    {
        std::lock_guard<std::mutex> lock{*m_window_lock};
        try
        {
            m_page->render();
        }
        catch (std::exception const& exp)
        {
            std::cerr << exp.what() << std::endl;
        }
    }
}

void window_manager::swap_page(std::shared_ptr<page> next_page)
{
    std::lock_guard<std::mutex> lock{*m_window_lock};

    if (m_page != nullptr)
    {
        m_page->close();
    }

    m_window_runner.reset();
    m_page = next_page;
    m_window_runner = std::make_unique<std::jthread>(&window_manager::display, this);
}
