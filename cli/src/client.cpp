#include <iostream>
#include <fstream>
#include <format>
#include <memory>
#include <flashback/client.hpp>
#include <grpcpp/grpcpp.h>

using namespace flashback;

client::client(std::shared_ptr<options> opts, std::shared_ptr<config_manager> config)
    : m_channel{
        grpc::CreateChannel(
            std::format("{}:{}", opts->server_address, opts->server_port),
            grpc::InsecureChannelCredentials())
    }
    , m_stub{Server::NewStub(m_channel)}
    , m_config{config}
    , m_user{nullptr}
{
    m_config->load();
    m_user = m_config->get_user();
}

bool client::user_is_defined() const noexcept
{
    return m_user != nullptr;
}

bool client::session_is_valid() const noexcept
{
    return user_is_defined() && !m_user->token().empty();
}

void client::user(std::shared_ptr<User> user)
{
    m_user = user;
}

void client::token(std::string token)
{
    m_user->set_token(std::move(token));
}

std::shared_ptr<Roadmaps> client::roadmaps()
{
    auto context{std::make_unique<grpc::ClientContext>()};
    context->AddMetadata("authorization", m_user->token());

    std::shared_ptr<Roadmaps> data{std::make_shared<Roadmaps>()};
    std::unique_ptr<User> user{std::make_unique<User>(*m_user)};

    auto request{std::make_shared<RoadmapsRequest>()};
    request->set_allocated_user(user.release());

    if (grpc::Status const status{m_stub->GetRoadmaps(context.get(), *request, data.get())}; !status.ok())
    {
        throw std::runtime_error{"Client: failed to retrieve roadmaps"};
    }

    return data;
}

std::shared_ptr<SignInResponse> client::signin()
{
    auto context{std::make_unique<grpc::ClientContext>()};
    auto request{std::make_unique<SignInRequest>()};
    auto response{std::make_shared<SignInResponse>()};

    if (m_user == nullptr)
    {
        response->set_success(false);
        response->set_details("invalid user");
    }
    else
    {
        std::clog << std::format("Client: signing in as {}\n", m_user->email());
        request->set_allocated_user(m_user.get());

        if (grpc::Status const status{m_stub->SignIn(context.get(), *request, response.get())}; status.ok())
        {
            if (response->success())
            {
                m_user->set_token(response->token());
                m_config->store(m_user);
            }
            else
            {
                std::cerr << response->details() << std::endl;
            }
        }
        else
        {
            throw std::runtime_error{std::format("Client: server error")};
        }
    }

    return response;
}

std::shared_ptr<SignUpResponse> client::signup()
{
    auto context{std::make_unique<grpc::ClientContext>()};
    auto request{std::make_shared<SignUpRequest>()};
    auto response{std::make_shared<SignUpResponse>()};

    if (m_user == nullptr)
    {
        response->set_success(false);
        response->set_details("user information incomplete");
    }
    else
    {
        std::clog << std::format("Client: signing up {}\n", m_user->email());
        request->set_allocated_user(m_user.get());

        if (grpc::Status const status{m_stub->SignUp(context.get(), *request, response.get())}; status.ok())
        {
            std::clog << std::format("Client: {}\n", response->details());
        }
        else
        {
            throw std::runtime_error{"Client: failed to sign up"};
        }
    }

    return response;
}
