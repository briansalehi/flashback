#include <iostream>
#include <format>
#include <flashback/server.hpp>
#include <sodium.h>

using namespace flashback;

server::server(std::shared_ptr<database> database)
    : m_database{database}
{
    if (sodium_init() < 0)
    {
        throw std::runtime_error("Server: sodium library cannot be initialized");
    }
}

grpc::Status server::GetRoadmaps(grpc::ServerContext* context, User const* request, Roadmaps* response)
{
    std::clog << std::format("Server: user {} requests for roadmaps\n", request->id());
    pqxx::result const result{m_database->query(std::format("select id, name from get_roadmaps({})", request->id()))};
    std::clog << std::format("Server: retrieving {} roadmaps for user {}\n", result.size(), request->id());

    for (pqxx::row row : result)
    {
        Roadmap* r{response->add_roadmaps()};
        r->set_id(row.at("id").as<std::uint64_t>());
        r->set_name(row.at("name").as<std::string>());
    }

    return grpc::Status::OK;
}

grpc::Status server::SignIn(grpc::ServerContext* context, const SignInRequest* request, SignInResponse* response)
{
    uint64_t const opslimit{crypto_pwhash_OPSLIMIT_MODERATE};
    size_t const memlimit{crypto_pwhash_MEMLIMIT_MODERATE};
    char buffer[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(buffer, request->user().password().c_str(), request->user().password().size(), opslimit, memlimit) != 0)
    {
        throw std::runtime_error("Server: sodium cannot create hash");
    }

    std::string hash{buffer};

    pqxx::result result = m_database->query(std::format("call create_session('{}', '{}')", request->user().email(), request->user().password()));

    response->set_success(true);
    response->set_token(hash);
    response->set_details("signin successful");

    return grpc::Status::OK;
}

grpc::Status server::SignUp(grpc::ServerContext* context, const SignUpRequest* request, SignUpResponse* response)
{
    response->set_success(true);
    response->set_details("signup successful");

    return grpc::Status::OK;
}
