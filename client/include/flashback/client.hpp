#pragma once

#include <grpcpp/grpcpp.h>
#include <server.grpc.pb.h>
#include <flashback/basic_client.hpp>
#include <flashback/config_manager.hpp>

namespace flashback
{
class client final: public basic_client
{
public:
    explicit client(std::string address, std::string port);
    ~client() override;
    bool signup(std::string name, std::string email, std::string password) override;

private:
    std::unique_ptr<grpc::ClientContext> m_client_context;
    std::shared_ptr<grpc::Channel> m_channel;
    std::unique_ptr<Server::Stub> m_stub;
    std::unique_ptr<flashback::config_manager> m_config;
};

} // flashback
