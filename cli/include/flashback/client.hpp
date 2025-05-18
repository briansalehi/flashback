#pragma once

#include <flashback/options.hpp>
#include <boost/asio.hpp>

namespace flashback
{
class client final
{
public:
    explicit client(flashback::options const& opts);
    virtual ~client();
private:
    boost::asio::io_context context;
    boost::asio::ip::tcp::socket server;
};
} // flashback