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
}

client::~client()
{
    m_user.release();
}

bool client::user_is_defined() const noexcept
{
    return m_user != nullptr;
}

bool client::session_is_valid() const noexcept
{
    return !m_token.empty();
}

void client::user(std::unique_ptr<User> user)
{
    m_user = std::move(user);
}

void client::token(std::string token)
{
    m_token = std::move(token);
}

std::shared_ptr<Roadmaps> client::roadmaps()
{
    auto context{std::make_unique<grpc::ClientContext>()};
    context->AddMetadata("authorization", m_token);

    std::shared_ptr<Roadmaps> data{std::make_shared<Roadmaps>()};
    std::unique_ptr<User> user{std::make_unique<User>(*m_user)};

    auto request{std::make_shared<RoadmapsRequest>()};
    request->set_allocated_user(user.release());

    if (grpc::Status const status{m_stub->GetRoadmaps(context.get(), *request, data.get())}; status.ok())
    {
        std::clog << std::format("Client: retrieved {} roadmaps\n", data->roadmap().size());
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
            std::clog << std::format("Client: token {}\n", response->details(), response->token());
            if (response->success())
            {
                std::clog << std::format("Client: {} with token {}\n", response->details(), response->token());
                m_token = response->token();
            }
            else
            {
                if (response->details() == "UD001")
                    throw std::runtime_error("account does not exist");
                else if (response->details() == "UD002")
                    throw std::runtime_error("incorrect password");
                else if (response->details() == "UD003")
                    throw std::runtime_error("already logged in");
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
        response->set_details("invalid user");
    }
    else
    {
        std::clog << std::format("Client: signing up {}\n", m_user->email());
        request->set_allocated_user(m_user.release());

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
