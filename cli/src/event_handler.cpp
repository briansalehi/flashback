#include <flashback/event_handler.hpp>
#include <flashback/welcome.hpp>

using namespace flashback;

event_handler::event_handler(std::shared_ptr<options> options)
    : m_server{std::make_shared<server>(options)}
    , m_page{std::make_shared<welcome>(m_server)}
{
    // bool credentials_valid{server->check_user_credentials()};
    // std::set<roadmap> const roadmaps{m_server->roadmaps(2)};
}

event_handler::~event_handler()
{
}
