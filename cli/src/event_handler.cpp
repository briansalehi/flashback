#include <flashback/event_handler.hpp>
#include <flashback/welcome.hpp>

using namespace flashback;

event_handler::event_handler(std::shared_ptr<options> options)
    : m_channel{grpc::CreateChannel(std::format("{}:{}", options->server_address, options->server_port), grpc::InsecureChannelCredentials())}
    , m_client{std::make_shared<client>(options, m_channel)}
    , m_page{nullptr}
{
    try
    {
        // bool credentials_valid{server->check_user_credentials()};
        // std::set<roadmap> const roadmaps{m_server->roadmaps(2)};
        // m_page = std::make_shared<welcome>(m_client);
        std::shared_ptr<SignUpRequest> signup_request{std::make_shared<SignUpRequest>()};
        signup_request->set_email("briansalehi@proton.me");
        signup_request->set_password("1234");
        std::shared_ptr<SignUpResponse> signup_response{m_client->signup(signup_request)};

        std::shared_ptr<SignInRequest> signin_request{std::make_shared<SignInRequest>()};
        signin_request->set_email("briansalehi@proton.me");
        signin_request->set_password("1234");
        std::shared_ptr<SignInResponse> signin_response{m_client->signin(signin_request)};
    }
    catch (std::runtime_error const& exp)
    {
        std::cerr << exp.what() << std::endl;
    }
}

event_handler::~event_handler()
{
}
