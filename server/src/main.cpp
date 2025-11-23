#include <memory>
#include <format>
#include <string>
#include <iostream>
#include <exception>
#include <flashback/server.hpp>
#include <flashback/database.hpp>
#include <grpcpp/grpcpp.h>

constexpr uint16_t server_port{9821};
constexpr std::string listen_address{"[::]"};

int main()
{
    try
    {
        auto database{std::make_shared<flashback::database>()};
        auto server{std::make_shared<flashback::server>(database)};
        auto builder{std::make_unique<grpc::ServerBuilder>()};
        builder->AddListeningPort(std::format("{}:{}", listen_address, server_port), grpc::InsecureServerCredentials());
        builder->RegisterService(server.get());
        std::unique_ptr<grpc::Server> service{builder->BuildAndStart()};
        service->Wait();
    }
    catch (std::exception const& exp)
    {
        std::cerr << "Unknown error: " << exp.what() << std::endl;
        std::exit(-1);
    }
}
