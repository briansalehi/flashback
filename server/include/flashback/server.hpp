#pragma once

#include <memory>
#include <types.pb.h>
#include <server.grpc.pb.h>
#include <flashback/database.hpp>

namespace flashback
{
class server final: public Server::Service
{
public:
    explicit server(std::shared_ptr<database> database);
    ~server() override = default;

    grpc::Status GetRoadmaps(grpc::ServerContext* context, User const* request, Roadmaps* response) override;
    grpc::Status SignIn(grpc::ServerContext* context, const SignInRequest* request, SignInResponse* response) override;
    grpc::Status SignUp(grpc::ServerContext* context, SignUpRequest const* request, SignUpResponse* response) override;

private:
    std::shared_ptr<database> m_database;
};
} // flashback
