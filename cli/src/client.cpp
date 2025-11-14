#include <iostream>
#include <format>
#include <memory>
#include <flashback/client.hpp>

using namespace flashback;

client::client(std::shared_ptr<options> opts, std::shared_ptr<grpc::Channel> channel)
    : m_stub{Server::NewStub(channel)}
{
    std::clog << std::format("Client: connecting to {}:{}\n", opts->server_address, opts->server_port);
}

std::shared_ptr<Roadmaps> client::get_roadmaps(std::shared_ptr<User> requester)
{
    auto context{std::make_unique<grpc::ClientContext>()};
    std::shared_ptr<Roadmaps> data{std::make_shared<Roadmaps>()};

    if (grpc::Status const status{m_stub->GetRoadmaps(context.get(), *requester, data.get())}; status.ok())
    {
        std::clog << std::format("Client: retrieved {} roadmaps\n", data->roadmaps().size());
    }
    else
    {
        throw std::runtime_error{"Client: failed to retrieve roadmaps"};
    }

    return data;
}

std::shared_ptr<SignInResponse> client::signin(std::shared_ptr<SignInRequest> request)
{
    auto context{std::make_unique<grpc::ClientContext>()};
    std::shared_ptr<SignInResponse> response{std::make_shared<SignInResponse>()};
    std::clog << std::format("Client: signing in as {}\n", request->email());

    if (grpc::Status const status{m_stub->SignIn(context.get(), *request, response.get())}; status.ok())
    {
        std::clog << std::format("Client: {} with token {}\n", response->details(), response->token());
    }
    else
    {
        throw std::runtime_error{"Client: failed to sign in"};
    }

    return response;
}

std::shared_ptr<SignUpResponse> client::signup(std::shared_ptr<SignUpRequest> request)
{
    auto context{std::make_unique<grpc::ClientContext>()};
    std::shared_ptr<SignUpResponse> response{std::make_shared<SignUpResponse>()};
    std::clog << std::format("Client: signing up as {}\n", request->email());

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
