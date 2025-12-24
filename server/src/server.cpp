#include <algorithm>
#include <chrono>
#include <flashback/server.hpp>
#include <flashback/exception.hpp>
#include <format>
#include <iostream>
#include <sodium.h>
#include <sstream>

using namespace flashback;

server::server(std::shared_ptr<basic_database> database)
    : m_database{database}
{
    if (sodium_init() < 0)
    {
        throw std::runtime_error("Server: sodium library cannot be initialized");
    }
}

grpc::Status server::SignIn(grpc::ServerContext* context, const SignInRequest* request, SignInResponse* response)
{
    try
    {
        bool const user_is_valid{request->has_user()};
        bool const email_is_set{!request->user().email().empty()};
        bool const password_is_set{!request->user().password().empty()};
        bool const device_is_set{!request->user().device().empty()};

        if (!user_is_valid || !email_is_set || !password_is_set || !device_is_set)
        {
            throw client_exception("incomplete credentials");
        }

        if (!m_database->user_exists(request->user().email()))
        {
            throw client_exception(std::format("user is not registered with email {}", request->user().email()));
        }

        auto user{m_database->get_user(request->user().email())};
        user->set_token(generate_token());
        user->set_device(request->user().device());

        if (!password_is_valid(user->hash(), request->user().password()))
        {
            throw client_exception("invalid credentials");
        }

        if (!m_database->create_session(user->id(), user->token(), user->device()))
        {
            throw client_exception(std::format("cannot create session for user {}", user->id()));
        }

        std::cerr << std::format("Client {}: signed in with device {}\n", user->id(), user->device());

        user->clear_id();
        user->clear_hash();
        user->clear_password();
        response->set_success(true);
        response->set_details("sign in successful");
        response->set_allocated_user(user.release());
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.code());
        std::cerr << std::format("Client: {}\n", exp.what());
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
    try
    {
        if (!request->has_user() || request->user().name().empty() || request->user().email().empty() || request->user().device().empty() ||
            request->user().password().empty())
        {
            throw client_exception("incomplete credentials");
        }

        if (m_database->user_exists(request->user().email()))
        {
            throw client_exception(std::format("user {} is already registered", request->user().email()));
        }

        auto user{std::make_unique<User>(request->user())};
        user->set_hash(calculate_hash(user->password()));
        uint64_t user_id{m_database->create_user(user->name(), user->email(), user->hash())};
        user->clear_password();
        user->clear_hash();
        user->clear_id();

        if (user_id > 0)
        {
            response->set_success(true);
            response->set_details("signup successful");
            response->set_allocated_user(user.release());
        }
        else
        {
            response->set_success(false);
            response->set_details("signup failed");
        }
    }
    catch (client_exception const& exp)
    {
        response->set_success(false);
        response->set_details(exp.code());
        std::cerr << std::format("Client: {}\n", exp.what());
    }
    catch (std::exception const& exp)
    {
        response->set_success(false);
        response->set_details("internal error");
        std::cerr << std::format("Server: {}\n", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::ResetPassword(grpc::ServerContext* context, ResetPasswordRequest const* request, ResetPasswordResponse* response)
{
    try
    {
        if (request->has_user() && (request->user().id() == 0 || request->user().email().empty() || request->user().password().empty() ||
            request->user().device().empty()))
        {
            throw client_exception("incomplete credentials");
        }

        std::string hash{calculate_hash(request->user().password())};

        auto user{std::make_unique<User>(request->user())};
        user->clear_password();
        user->clear_device();

        m_database->reset_password(user->id(), hash);

        response->set_allocated_user(user.release());
    }
    catch (std::exception const& exp)
    {
        std::cerr << exp.what() << std::endl;
    }

    return grpc::Status::OK;
}

grpc::Status server::VerifySession(grpc::ServerContext* context, VerifySessionRequest const* request, VerifySessionResponse* response)
{
    try
    {
        response->set_valid(request->has_user() && session_is_valid(request->user()));
    }
    catch (std::exception const& exp)
    {
        response->set_valid(false);
    }

    return grpc::Status::OK;
}

grpc::Status server::CreateRoadmap(grpc::ServerContext* context, CreateRoadmapRequest const* request, CreateRoadmapResponse* response)
{
    response->clear_roadmap();

    try
    {
        if (request->has_user() && session_is_valid(request->user()))
        {
            auto roadmap{std::make_unique<Roadmap>()};
            roadmap->set_name(request->name());
            roadmap->set_id(m_database->create_roadmap(request->name()));
            response->set_allocated_roadmap(roadmap.release());
        }
    }
    catch (client_exception const& exp)
    {
        std::cerr << std::format("Client: {}", exp.what());
    }
    catch (pqxx::unique_violation const& exp)
    {
        std::cerr << std::format("Server: {}", exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::AssignRoadmap(grpc::ServerContext* context, AssignRoadmapRequest const* request, AssignRoadmapResponse* response)
{
    return grpc::Status::OK;
}

grpc::Status server::GetRoadmaps(grpc::ServerContext* context, GetRoadmapsRequest const* request, GetRoadmapsResponse* response)
{
    try
    {
        if (!request->has_user() || request->user().token().empty() || request->user().device().empty())
        {
            throw client_exception("incomplete credentials");
        }

        std::shared_ptr<User> user{m_database->get_user(request->user().token(), request->user().device())};

        if (nullptr == user)
        {
            throw client_exception("unauthorized request for roadmaps");
        }

        std::vector<Roadmap> roadmaps{m_database->get_roadmaps(user->id())};
        std::clog << std::format("Client {}: collected {} roadmaps\n", request->user().id(), roadmaps.size());

        for (Roadmap roadmap: roadmaps)
        {
            auto allocated_roadmap = response->add_roadmap();
            allocated_roadmap->set_id(roadmap.id());
            allocated_roadmap->set_name(roadmap.name());
        }
    }
    catch (client_exception const& exp)
    {
        std::cerr << std::format("Client {}: {}\n", request->user().id(), exp.what());
    }
    catch (std::exception const& exp)
    {
        std::cerr << std::format("Server: failed to collect roadmaps for client {} because:\n{}\n", request->user().id(), exp.what());
    }

    return grpc::Status::OK;
}

grpc::Status server::RenameRoadmap(grpc::ServerContext* context, RenameRoadmapRequest const* request, RenameRoadmapResponse* response)
{
    return grpc::Status::OK;
}

grpc::Status server::RemoveRoadmap(grpc::ServerContext* context, RemoveRoadmapRequest const* request, RemoveRoadmapResponse* response)
{
    return grpc::Status::OK;
}

grpc::Status server::SearchRoadmaps(grpc::ServerContext* context, SearchRoadmapsRequest const* request, SearchRoadmapsResponse* response)
{
    return grpc::Status::OK;
}

std::string server::calculate_hash(std::string_view password)
{
    char buffer[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(buffer, password.data(), password.size(), crypto_pwhash_OPSLIMIT_MODERATE, crypto_pwhash_MEMLIMIT_MODERATE) != 0)
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

bool server::session_is_valid(User const& user) const
{
    return nullptr != m_database->get_user(user.token(), user.device());
}

