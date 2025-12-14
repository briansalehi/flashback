#include <flashback/client.hpp>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>

using namespace flashback;

client::client(std::shared_ptr<options> opts, std::shared_ptr<config_manager> config)
    : m_channel{
          grpc::CreateChannel(std::format("{}:{}", opts->server_address, opts->server_port),
                              grpc::InsecureChannelCredentials())
      },
      m_stub{Server::NewStub(m_channel)}, m_config{config}, m_user{nullptr}
{
    m_config->load();
    m_user = m_config->get_user();
}

bool client::user_is_defined() const noexcept { return m_user != nullptr; }

bool client::session_is_valid() const noexcept
{
    auto valid{false};
    auto context{std::make_unique<grpc::ClientContext>()};
    auto request{std::make_shared<VerifySessionRequest>()};
    auto response{std::make_shared<VerifySessionResponse>()};

    if (user_is_defined() && !m_user->token().empty() && m_user->id() > 0)
    {
        std::unique_ptr<User> user{std::make_unique<User>(*m_user)};
        request->set_allocated_user(user.release());

        if (grpc::Status const status{m_stub->VerifySession(context.get(), *request, response.get())}; status.ok())
        {
            valid = response->valid();
        }
    }

    return valid;
}

void client::user(std::shared_ptr<User> user) { m_user = user; }

void client::token(std::string token) { m_user->set_token(std::move(token)); }

std::shared_ptr<Roadmaps> client::roadmaps()
{
    auto context{create_context()};

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
        response->set_details("undefined user");
    }
    else
    {
        if (m_user->device().empty())
        {
            m_user->set_device(config_manager::create_device_id());
        }

        std::clog << std::format("Client: signing in as {} with device {}\n", m_user->email(), m_user->device());

        auto request_user{std::make_unique<User>(*m_user)};
        request->set_allocated_user(request_user.release());

        if (grpc::Status const status{m_stub->SignIn(context.get(), *request, response.get())}; status.ok())
        {
            if (response->success())
            {
                m_user = std::make_shared<User>(response->user());

                std::clog << std::format("Client: {}\n", response->details());

                if (response->success())
                {
                    m_config->store(m_user);
                }
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
        m_user->set_device(config_manager::create_device_id());
        std::clog << std::format("Client: signing up as {}\n", m_user->email());
        request->set_allocated_user(m_user.get());

        if (grpc::Status const status{m_stub->SignUp(context.get(), *request, response.get())}; status.ok())
        {
            m_user = std::make_shared<User>(response->user());

            std::clog << std::format("Client: {}\n", response->details());

            if (response->success())
            {
                m_config->store(m_user);
            }
        }
        else
        {
            throw std::runtime_error{"Client: failed to sign up"};
        }
    }

    return response;
}

std::shared_ptr<ResetPasswordResponse> client::reset_password()
{
    auto context{std::make_unique<grpc::ClientContext>()};
    auto request{std::make_shared<ResetPasswordRequest>()};
    auto response{std::make_shared<ResetPasswordResponse>()};

    if (m_user == nullptr)
    {
        response->set_success(false);
        response->set_details("user undefined");
    }
    else
    {
        request->set_allocated_user(m_user.get());

        if (grpc::Status const status{m_stub->ResetPassword(context.get(), *request, response.get())}; status.ok())
        {
            m_user = std::make_shared<User>(response->user());

            std::clog << std::format("Client: {}\n", response->details());

            if (response->success())
            {
                m_config->store(m_user);
            }
        }
        else
        {
            throw std::runtime_error{"Client: failed to sign up"};
        }
    }

    return response;
}

std::unique_ptr<grpc::ClientContext> client::create_context()
{
    auto context{std::make_unique<grpc::ClientContext>()};
    context->AddMetadata("email", m_user->email());
    context->AddMetadata("device", m_user->device());
    context->AddMetadata("token", m_user->token());

    return context;
}
