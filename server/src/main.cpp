#include <memory>
#include <iostream>
#include <exception>
#include <cstdlib>
#include <format>
#include <flashback/server.hpp>
#include <flashback/database.hpp>
#include <grpcpp/grpcpp.h>

int main()
{
    try
    {
        auto database{std::make_shared<flashback::database>()};
        auto server_impl{std::make_shared<flashback::server_impl>(9821, database)};
        grpc::ServerBuilder builder{};
        builder.AddListeningPort("localhost:9821", grpc::InsecureServerCredentials());
        builder.RegisterService(server_impl.get());
        std::unique_ptr<grpc::Server> server{builder.BuildAndStart()};
        server->Wait();
    }
    catch (std::exception const& exp)
    {
        std::cerr << "Unknown error: " << exp.what() << std::endl;
        std::exit(-1);
    }
}
