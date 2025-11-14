#include <flashback/window_manager.hpp>
#include <flashback/welcome_page.hpp>

using namespace flashback;

window_manager::window_manager(std::shared_ptr<client> client, std::shared_ptr<User> user)
    : m_client{client}
    , m_page{nullptr}
{
    try
    {
        // bool credentials_valid{server->check_user_credentials()};
        // std::set<roadmap> const roadmaps{m_server->roadmaps(2)};
        // m_page = std::make_shared<welcome_page>(m_client);
        auto signup_request{std::make_shared<SignUpRequest>()};
        std::shared_ptr<SignUpResponse> signup_response{m_client->signup(user)};

        auto signin_request{std::make_shared<SignInRequest>()};
        std::shared_ptr<SignInResponse> signin_response{m_client->signin(user)};
    }
    catch (std::runtime_error const& exp)
    {
        std::cerr << exp.what() << std::endl;
    }
}
