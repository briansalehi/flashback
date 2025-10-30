#include <flashback/client.hpp>
#include <iostream>
#include <format>

using namespace flashback;

client::client(std::shared_ptr<options> opts)
    : m_server{m_context}
{
    boost::asio::ip::tcp::resolver::query const query{opts->server_address, opts->server_port};

    for (boost::asio::ip::tcp::resolver resolver{m_context};
         boost::asio::ip::tcp::endpoint endpoint : resolver.resolve(query))
    {
        m_server.connect(endpoint);
        if (m_server.is_open())
        {
            break;
        }
    }

    std::clog << std::format("Connected to [{}]:{}", m_server.remote_endpoint().address().to_string(), m_server.remote_endpoint().port());
}

client::~client()
{
    if (m_server.is_open())
    {
        m_server.close();
    }
}