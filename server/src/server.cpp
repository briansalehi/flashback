#include <iostream>
#include <format>
#include <flashback/server.hpp>

using namespace flashback;

server_impl::server_impl(boost::asio::ip::port_type const port, std::shared_ptr<database> database)
    : m_database{database}
    , m_runner{nullptr}
{
}

server_impl::~server_impl()
{
}

grpc::Status server_impl::get_roadmaps(grpc::ServerContext* context, user const* request, roadmaps* response)
{
    std::clog << std::format("Server: user {} requests for roadmaps\n", request->id());
    pqxx::result const result{m_database->query(std::format("select id, name from get_roadmaps({})", request->id()))};
    std::clog << std::format("Server: retrieving {} roadmaps for user {}\n", result.size(), request->id());

    for (pqxx::row row : result)
    {
        roadmap* r{response->add_roadmap()};
        r->set_id(row.at("id").as<std::uint64_t>());
        r->set_name(row.at("name").as<std::string>());
    }

    return grpc::Status::OK;
}
