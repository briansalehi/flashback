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

int main(int const argc, char** argv)
{
    try
    {
        std::string database_host{std::getenv("DATABASE_HOST") ? std::getenv("DATABASE_HOST") : "localhost"};
        auto database{std::make_shared<flashback::database>("flashback_client", "flashback", database_host, "5432")};
        auto const server{std::make_shared<flashback::server>(database)};
        auto const builder{std::make_unique<grpc::ServerBuilder>()};

        // grpc::SslServerCredentialsOptions opts;
        // opts.pem_key_cert_pairs.push_back({key, cert});
        // std::shared_ptr<grpc::ServerCredentials> credentials{grpc::SslServerCredentials(opts)};
        std::shared_ptr<grpc::ServerCredentials> const credentials{grpc::InsecureServerCredentials()};
        builder->AddListeningPort(std::format("{}:{}", listen_address, server_port), credentials);
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
