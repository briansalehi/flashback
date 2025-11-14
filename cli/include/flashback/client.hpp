#pragma once

#include <memory>
#include <grpcpp/grpcpp.h>
#include <flashback/options.hpp>
#include <types.pb.h>
#include <server.grpc.pb.h>

namespace flashback
{
class client final
{
public:
    explicit client(std::shared_ptr<options> opts, std::shared_ptr<grpc::Channel> channel);
    ~client() = default;

    std::shared_ptr<Roadmaps> get_roadmaps(std::shared_ptr<User> requester);
    std::shared_ptr<SignInResponse> signin(std::shared_ptr<SignInRequest> request);
    std::shared_ptr<SignUpResponse> signup(std::shared_ptr<SignUpRequest> request);

private:
    std::unique_ptr<Server::Stub> m_stub;
};
} // flashback