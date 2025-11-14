#pragma once

#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include <flashback/options.hpp>
#include <types.pb.h>
#include <server.grpc.pb.h>

namespace flashback
{
class client final
{
public:
    explicit client(std::shared_ptr<options> opts);
    ~client() = default;

    std::shared_ptr<Roadmaps> get_roadmaps(std::shared_ptr<User> user);
    std::shared_ptr<SignInResponse> signin(std::shared_ptr<User> user);
    std::shared_ptr<SignUpResponse> signup(std::shared_ptr<User> user);

private:
    std::shared_ptr<grpc::Channel> m_channel;
    std::unique_ptr<Server::Stub> m_stub;
    std::shared_ptr<User> m_user;
    std::string m_token;
};
} // flashback