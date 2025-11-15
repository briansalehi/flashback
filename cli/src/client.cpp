#include <iostream>
#include <format>
#include <memory>
#include <flashback/client.hpp>
#include <grpcpp/grpcpp.h>

using namespace flashback;

client::client(std::shared_ptr<options> opts)
    : m_channel{grpc::CreateChannel(
        std::format("{}:{}", opts->server_address, opts->server_port),
        grpc::InsecureChannelCredentials())}
    , m_stub{Server::NewStub(m_channel)}
    , m_user{nullptr}
{
    std::clog << std::format("Client: connecting to {}:{}\n", opts->server_address, opts->server_port);
}

client::~client()
{
    m_user.release();
}

bool client::user_is_defined() const noexcept
{
    return m_user != nullptr;
}

void client::set_user(std::unique_ptr<User> user)
{
    m_user = std::move(user);
}

std::shared_ptr<Roadmaps> client::get_roadmaps()
{
    auto context{std::make_unique<grpc::ClientContext>()};
    context->AddMetadata("authorization", m_token);

    std::shared_ptr<Roadmaps> data{std::make_shared<Roadmaps>()};

    if (grpc::Status const status{m_stub->GetRoadmaps(context.get(), *m_user, data.get())}; status.ok())
    {
        std::clog << std::format("Client: retrieved {} roadmaps\n", data->roadmaps().size());
    }
    else
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
        request->set_allocated_user(m_user.release());

        if (grpc::Status const status{m_stub->SignIn(context.get(), *request, response.get())}; status.ok())
        {
            std::clog << std::format("Client: {} with token {}\n", response->details(), response->token());
            m_token = response->token();
        }
        else
        {
            throw std::runtime_error{std::format("Client: {}", response->details())};
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
        response->set_details("invalid user");
    }
    else
    {
        request->set_allocated_user(m_user.release());
        std::clog << std::format("Client: signing up {}\n", m_user->email());

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
