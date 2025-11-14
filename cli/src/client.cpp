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
    m_user = std::make_shared<User>();
    m_user->set_name("Brian Salehi");
    m_user->set_email("briansalehi@proton.me");
    m_user->set_password("1234");

    std::clog << std::format("Client: connecting to {}:{}\n", opts->server_address, opts->server_port);

    if (m_user != nullptr)
    {
    }

    if (m_token.empty())
    {
        signin(m_user);
    }
}

std::shared_ptr<Roadmaps> client::get_roadmaps(std::shared_ptr<User> user)
{
    auto context{std::make_unique<grpc::ClientContext>()};
    context->AddMetadata("authorization", m_token);

    std::shared_ptr<Roadmaps> data{std::make_shared<Roadmaps>()};

    if (grpc::Status const status{m_stub->GetRoadmaps(context.get(), *user, data.get())}; status.ok())
    {
        std::clog << std::format("Client: retrieved {} roadmaps\n", data->roadmaps().size());
    }
    else
    {
        throw std::runtime_error{"Client: failed to retrieve roadmaps"};
    }

    return data;
}

std::shared_ptr<SignInResponse> client::signin(std::shared_ptr<User> user)
{
    auto context{std::make_unique<grpc::ClientContext>()};
    auto request{std::make_unique<SignInRequest>()};
    auto response{std::make_shared<SignInResponse>()};

    request->set_allocated_user(user.get());
    std::clog << std::format("Client: signing in {}\n", user->email());

    if (grpc::Status const status{m_stub->SignIn(context.get(), *request, response.get())}; status.ok())
    {
        std::clog << std::format("Client: {} with token {}\n", response->details(), response->token());
        m_token = response->token();
    }
    else
    {
        throw std::runtime_error{"Client: failed to sign in"};
    }

    return response;
}

std::shared_ptr<SignUpResponse> client::signup(std::shared_ptr<User> user)
{
    auto context{std::make_unique<grpc::ClientContext>()};
    auto request{std::make_shared<SignUpRequest>()};
    auto response{std::make_shared<SignUpResponse>()};

    request->set_allocated_user(user.get());
    std::clog << std::format("Client: signing up {}\n", user->email());

    if (grpc::Status const status{m_stub->SignUp(context.get(), *request, response.get())}; status.ok())
    {
        std::clog << std::format("Client: {}\n", response->details());
    }
    else
    {
        throw std::runtime_error{"Client: failed to sign up"};
    }

    return response;
}
