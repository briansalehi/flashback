#pragma once

#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include <flashback/options.hpp>
#include <flashback/config_manager.hpp>
#include <types.pb.h>
#include <server.grpc.pb.h>

namespace flashback
{
class client final
{
public:
    explicit client(std::shared_ptr<options> opts, std::shared_ptr<config_manager> config);
    ~client() = default;

    [[nodiscard]] bool user_is_defined() const noexcept;
    [[nodiscard]] bool session_is_valid() const noexcept;
    void user(std::shared_ptr<User> user);
    void token(std::string user);

    std::shared_ptr<Roadmaps> roadmaps();
    std::shared_ptr<SignInResponse> signin();
    std::shared_ptr<SignUpResponse> signup();

private:
    std::shared_ptr<grpc::Channel> m_channel;
    std::unique_ptr<Server::Stub> m_stub;
    std::shared_ptr<config_manager> m_config;
    std::shared_ptr<User> m_user;
    std::string m_token;
};
} // flashback