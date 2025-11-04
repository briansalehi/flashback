#include <flashback/event_handler.hpp>
#include <flashback/welcome.hpp>

using namespace flashback;

event_handler::event_handler(std::shared_ptr<options> options)
    : m_channel{grpc::CreateChannel(std::format("{}:{}", options->server_address, options->server_port), grpc::InsecureChannelCredentials())}
    , m_client{std::make_shared<client>(options, m_channel)}
    , m_page{nullptr}
{
    // bool credentials_valid{server->check_user_credentials()};
    // std::set<roadmap> const roadmaps{m_server->roadmaps(2)};
    m_page = std::make_shared<welcome>(m_client);
}

event_handler::~event_handler()
{
}
