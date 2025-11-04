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
    ~client();

    std::shared_ptr<roadmaps> get_roadmaps(std::shared_ptr<user> requester);

private:
    std::unique_ptr<server::Stub> m_stub;
};
} // flashback