#pragma once

#include <memory>
#include <flashback/options.hpp>
#include <boost/asio.hpp>

namespace flashback
{
class client final
{
public:
    explicit client(std::shared_ptr<options> opts);
    ~client();

private:
    boost::asio::io_context m_context;
    boost::asio::ip::tcp::socket m_server;
};
} // flashback