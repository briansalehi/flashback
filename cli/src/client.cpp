#include <iostream>
#include <format>
#include <memory>
#include <flashback/client.hpp>

using namespace flashback;

client::client(std::shared_ptr<options> opts, std::shared_ptr<grpc::Channel> channel)
    : m_stub{server::NewStub(channel)}
{
    std::clog << std::format("Connecting to {}:{}\n", opts->server_address, opts->server_port);
}

client::~client()
{
}

std::shared_ptr<roadmaps> client::get_roadmaps(std::shared_ptr<user> requester)
{
    std::shared_ptr<roadmaps> data{std::make_shared<roadmaps>()};
    grpc::ClientContext context{};

    if (grpc::Status const status{m_stub->get_roadmaps(&context, *requester, data.get())}; status.ok())
    {
        std::clog << std::format("Client: retrieved {} roadmaps\n", data->roadmap().size());
    }
    else
    {
        std::cerr << "Client: failed to retrieve roadmaps from server\n";
    }

    return data;
}
