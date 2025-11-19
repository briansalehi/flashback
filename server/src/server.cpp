#include <iostream>
#include <format>
#include <chrono>
#include <sstream>
#include <cstring>
#include <algorithm>
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

grpc::Status server::GetRoadmaps(grpc::ServerContext* context, RoadmapsRequest const* request, Roadmaps* response)
{
    try
    {
        std::clog << std::format("Server: user {} requests for roadmaps\n", request->user().id());
        std::shared_ptr<Roadmaps> result{m_database->get_roadmaps(request->user().id())};
        std::clog << std::format("Server: collected {} roadmaps for user {}\n", result->roadmap().size(),
                                 request->user().id());

        for (Roadmap const& r : result->roadmap())
        {
            Roadmap* roadmap{response->add_roadmap()};
            roadmap->set_id(r.id());
            roadmap->set_name(r.name());
        }
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: failed to collect roadmaps for client {}", request->user().id());
    }

    return grpc::Status::OK;
}

grpc::Status server::SignIn(grpc::ServerContext* context, const SignInRequest* request, SignInResponse* response)
{
    try
    {
        std::unique_ptr<User> user{m_database->get_user(request->user().email())};

        if (user == nullptr)
        {
            throw client_exception("user is not registered");
        }

        if (!password_is_valid(user->hash(), request->user().password()))
        {
            throw client_exception("invalid credentials");
        }

        std::string token{generate_token()};
        std::string device{request->user().device()};

        std::cerr << std::format("Server: user {} signed in with device {}\n", user->id(), device);

        std::pair<bool, std::string> session_result{m_database->create_session(user->id(), token, device)};

        response->set_success(true);
        response->set_token(token);
        response->set_details("sign in successful");
        response->set_allocated_user(user.release());
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.code());
        std::cerr << std::format("Server: {}\n", exp.what());
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        std::cerr << std::format("Server: {}\n", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::SignUp(grpc::ServerContext* context, const SignUpRequest* request, SignUpResponse* response)
{
    response->set_success(true);
    response->set_details("signup successful");

    return grpc::Status::OK;
}

grpc::Status server::ResetPassword(grpc::ServerContext* context, ResetPasswordRequest const* request,
                                   ResetPasswordResponse* response)
{
    return grpc::Status::OK;
}

std::string server::calculate_hash(std::string_view password)
{
    uint64_t const opslimit{crypto_pwhash_OPSLIMIT_MODERATE};
    size_t const memlimit{crypto_pwhash_MEMLIMIT_MODERATE};
    char buffer[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(buffer, password.data(), password.size(), opslimit, memlimit) != 0)
    {
        throw std::runtime_error("Server: sodium cannot create hash");
    }

    return buffer;
}

bool server::password_is_valid(std::string_view hash, std::string_view password)
{
    return crypto_pwhash_str_verify(hash.data(), password.data(), password.size()) == 0;
}

std::string server::generate_token()
{
    unsigned char entropy[32];
    char token[64];

    randombytes_buf(entropy, sizeof(entropy));
    sodium_bin2base64(token, sizeof(token), entropy, sizeof(entropy), sodium_base64_VARIANT_ORIGINAL_NO_PADDING);

    return std::string{token};
}
