#pragma once

#include <memory>
#include <thread>
#include <boost/asio.hpp>
#include <types.pb.h>
#include <server.grpc.pb.h>
#include <flashback/database.hpp>

namespace flashback
{
class server_impl: public server::Service
{
public:
    explicit server_impl(boost::asio::ip::port_type const port, std::shared_ptr<database> database);
    ~server_impl();

    grpc::Status get_roadmaps(grpc::ServerContext* context, user const* request, roadmaps* response) override;

private:
    std::shared_ptr<database> m_database;
    std::unique_ptr<std::jthread> m_runner;
};
} // flashback
