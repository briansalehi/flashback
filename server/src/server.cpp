#include <algorithm>
#include <chrono>
#include <flashback/server.hpp>
#include <flashback/exception.hpp>
#include <format>
#include <iostream>
#include <sodium.h>
#include <sstream>

using namespace flashback;

server::server(std::shared_ptr<database> database) : m_database{database}
{
    if (sodium_init() < 0)
    {
        throw std::runtime_error("Server: sodium library cannot be initialized");
    }
}

grpc::Status server::GetRoadmaps(grpc::ServerContext* context, GetRoadmapsRequest const* request, GetRoadmapsResponse* response)
{
    try
    {
        std::multimap<grpc::string_ref, grpc::string_ref> metadata = context->client_metadata();
        std::string email, device, token;

        for (std::pair<grpc::string_ref, grpc::string_ref> const field: metadata)
        {
            if (field.first == "email") email = std::string{field.second.begin(), field.second.end()};
            else if (field.first == "device") device = std::string{field.second.begin(), field.second.end()};
            else if (field.first == "token") token = std::string{field.second.begin(), field.second.end()};
        }

        if (email.empty() || device.empty() || token.empty() || !session_is_valid(email, device, token))
        {
            throw client_exception("unauthorized request for roadmaps");
        }

        std::shared_ptr<Roadmaps> result{m_database->get_roadmaps(request->user().id())};
        std::clog << std::format("Client {}: collected {} roadmaps\n", request->user().id(), result->roadmap().size());

        for (Roadmap const& r : result->roadmap())
        {
            Roadmap* roadmap{response->add_roadmap()};
            roadmap->set_id(r.id());
            roadmap->set_name(r.name());
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

grpc::Status server::SignIn(grpc::ServerContext* context, const SignInRequest* request, SignInResponse* response)
{
    try
    {
        std::unique_ptr<User> user{m_database->get_user(request->user().email())};

        if (user == nullptr)
        {
            throw client_exception("user is not registered");
        }

        if (request->user().email().empty() || request->user().password().empty() || request->user().device().empty())
        {
            throw client_exception("incomplete credentials");
        }

        if (!password_is_valid(user->hash(), request->user().password()))
        {
            throw client_exception("invalid credentials");
        }

        user->set_token(generate_token());
        user->set_device(request->user().device());

        std::pair<bool, std::string> session_result{
            m_database->create_session(user->id(), user->token(), user->device())};

        if (!session_result.first)
        {
            throw client_exception(session_result.second);
        }

        std::cerr << std::format("Client {}: signed in with device {}\n", user->id(), user->device());

        auto returning_user{std::make_unique<User>(*user)};

        response->set_success(true);
        response->set_details("sign in successful");
        response->set_allocated_user(returning_user.release());
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
        std::unique_ptr<User> user{m_database->get_user(request->user().email())};

        if (user != nullptr)
        {
            throw client_exception(std::format("user {} is already registered", user->id()));
        }

        if (request->user().name().empty() || request->user().email().empty() || request->user().password().empty())
        {
            throw client_exception("incomplete credentials");
        }

        user = std::make_unique<User>();
        user->set_name(request->user().name());
        user->set_email(request->user().email());
        user->set_device(request->user().device());
        user->set_hash(calculate_hash(request->user().password()));

        uint64_t user_id{m_database->create_user(user->name(), user->email(), user->hash())};
        user->set_id(user_id);

        if (user->id() > 0)
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
        if (request->user().id() == 0 || request->user().email().empty() || request->user().password().empty() || request->user().device().empty())
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
        response->set_valid(session_is_valid(request->email(), request->device(), request->token()));
    }
    catch (std::exception const& exp)
    {
        response->set_valid(false);
    }

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

bool server::session_is_valid(std::string_view email, std::string_view device, std::string_view token) const
{
    bool is_valid{false};

    if (std::unique_ptr<User> user{m_database->get_user(email, device)}; user != nullptr)
    {
        is_valid = user->token() == token;
    }

    return is_valid;
}
