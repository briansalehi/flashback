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
    explicit client(std::string address, std::string port, std::shared_ptr<config_manager> config);
    ~client() override = default;
    bool signup(std::string name, std::string email, std::string password) override;
    bool signin(std::string email, std::string password) override;
    bool needs_to_signup() const override;
    bool needs_to_signin() const override;
    std::vector<Roadmap> get_roadmaps() const override;

private:
    std::shared_ptr<grpc::Channel> m_channel;
    std::unique_ptr<Server::Stub> m_stub;
    std::shared_ptr<config_manager> m_config;
};
} // flashback
