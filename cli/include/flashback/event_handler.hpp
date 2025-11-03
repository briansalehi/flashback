#pragma once

#include <memory>
#include <flashback/page.hpp>
#include <flashback/server.hpp>
#include <flashback/options.hpp>

namespace flashback
{
class event_handler
{
public:
    explicit event_handler(std::shared_ptr<options> options);
    ~event_handler();

private:
    std::shared_ptr<server> m_server;
    std::shared_ptr<page> m_page;
};
} // flashback
