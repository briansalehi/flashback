#include <flashback/client.hpp>
#include <iostream>
#include <format>

using namespace flashback;

client::client(flashback::options const& opts)
    : context{}
    , server{context}
{
    boost::asio::ip::tcp::resolver::query const query{opts.server_address, opts.server_port};

    for (boost::asio::ip::tcp::resolver resolver{context};
         boost::asio::ip::tcp::endpoint endpoint : resolver.resolve(query))
    {
        server.connect(endpoint);
        if (server.is_open())
        {
            break;
        }
    }

    std::clog << std::format("Connected to [{}]:{}", server.remote_endpoint().address().to_string(), server.remote_endpoint().port());
}

client::~client()
{
    if (server.is_open())
    {
        server.close();
    }
}