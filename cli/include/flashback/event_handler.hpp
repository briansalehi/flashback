#pragma once

#include <memory>
#include <flashback/page.hpp>
#include <flashback/client.hpp>
#include <flashback/options.hpp>
#include <grpcpp/grpcpp.h>

namespace flashback
{
class event_handler
{
public:
    explicit event_handler(std::shared_ptr<options> options);
    ~event_handler();

private:
    std::shared_ptr<grpc::Channel> m_channel;
    std::shared_ptr<client> m_client;
    std::shared_ptr<page> m_page;
};
} // flashback
