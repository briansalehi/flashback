#include <flashback/window_manager.hpp>
#include <flashback/welcome_page.hpp>

using namespace flashback;

window_manager::window_manager(std::shared_ptr<client> client)
    : m_client{client}
    , m_page{nullptr}
    , m_window_lock{std::make_unique<std::mutex>()}
    , m_window_runner{std::make_unique<std::jthread>(&window_manager::display, this)}
{
    try
    {
        std::shared_ptr<SignUpResponse> signup_response{m_client->signup()};
        std::shared_ptr<SignInResponse> signin_response{m_client->signin()};
    }
    catch (std::runtime_error const& exp)
    {
        std::cerr << exp.what() << std::endl;
    }
}

void window_manager::display()
{
    std::lock_guard<std::mutex> lock{*m_window_lock};


}
