#include <chrono>
#include <flashback/roadmap_page.hpp>
#include <flashback/signin_page.hpp>
#include <flashback/signup_page.hpp>
#include <flashback/welcome_page.hpp>
#include <flashback/window_manager.hpp>

using namespace flashback;

window_manager::window_manager(std::shared_ptr<client> client)
    : m_client{client}
    , m_page{nullptr}
    , m_next_page{nullptr}
    , m_window_lock{std::make_unique<std::mutex>()}
    , m_window_runner{nullptr}
{
}

window_manager::~window_manager()
{
    if (m_window_runner->joinable()) m_window_runner->join();

    m_window_runner.reset();
}

void window_manager::start()
{
    try
    {
        // swap_page(std::make_shared<welcome_page>());
        if (m_client->needs_to_signup())
        {
            display_signup();
        }
        else if (m_client->needs_to_signin())
        {
            display_signin();
        }
        else
        {
            display_roadmaps();
        }
    }
    catch (std::runtime_error const& exp)
    {
        m_page->close();
        std::cerr << exp.what() << std::endl;
    }
}

void window_manager::display_signup()
{
    std::shared_ptr window{shared_from_this()};
    auto const next_page{std::make_shared<signup_page>(m_client, window)};
    swap_page(next_page);
}

void window_manager::display_signin()
{
    std::shared_ptr window{shared_from_this()};
    auto const next_page{std::make_shared<signin_page>(m_client, window)};
    swap_page(next_page);
}

void window_manager::display_roadmaps()
{
    swap_page(std::make_shared<roadmap_page>(m_client, shared_from_this()));
}

void window_manager::display()
{
    if (m_page != nullptr)
    {
        try
        {
            // std::lock_guard<std::mutex> lock{*m_window_lock};
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
    // std::lock_guard<std::mutex> lock{*m_window_lock};

    if (m_page != nullptr)
    {
        m_page->close();
    }

    // m_window_runner.reset();
    m_page = next_page;
    display();
    // m_window_runner = std::make_unique<std::jthread>(&window_manager::display, this);
}
