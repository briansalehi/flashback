#include <format>
#include <flashback/client.hpp>

using namespace flashback;

client::client(std::string address, std::string port, std::shared_ptr<config_manager> config)
    : m_channel{grpc::CreateChannel(std::format("{}:{}", std::move(address), std::move(port)), grpc::InsecureChannelCredentials())}
    , m_stub{Server::NewStub(m_channel)}
    , m_config{config}
{
}

bool client::signup(std::string name, std::string email, std::string password)
{
    SignUpRequest request{};
    SignUpResponse response{};
    grpc::ClientContext context{};
    auto user{std::make_unique<User>()};
    std::string device{m_config->create_device_id()};
    user->set_name(std::move(name));
    user->set_email(std::move(email));
    user->set_device(std::move(device));
    user->set_password(std::move(password));
    request.set_allocated_user(user.release());
    m_stub->SignUp(&context, request, &response);
    bool const valid{response.success() && response.has_user()};
    auto const created_user{std::make_shared<User>()};
    created_user->set_name(std::move(name));
    created_user->set_email(std::move(email));
    created_user->set_device(std::move(device));
    created_user->set_token(std::move(""));

    if (response.has_user() && created_user)
    {
        m_config->store(created_user);
    }

    return valid;
}

bool client::signin(std::string email, std::string password)
{
    SignInRequest request{};
    SignInResponse response{};
    grpc::ClientContext context{};
    std::string device{m_config->create_device_id()};
    auto user{std::make_unique<User>()};
    user->set_email(email);
    user->set_device(device);
    user->set_password(std::move(password));
    request.set_allocated_user(user.release());
    m_stub->SignIn(&context, request, &response);
    bool const valid{response.success() && response.has_user() && !response.user().token().empty()};

    if (valid)
    {
        auto const created_user{std::make_shared<User>(response.user())};
        m_config->store(created_user);
    }

    return valid;
}

bool client::needs_to_signup() const
{
    return !m_config->has_credentials();
}

bool client::needs_to_signin() const
{
    VerifySessionRequest request{};
    VerifySessionResponse response{};
    grpc::ClientContext context{};
    response.clear_valid();
    request.set_allocated_user(m_config->get_user().release());

    if (m_config->has_credentials())
    {
        m_stub->VerifySession(&context, request, &response);
    }

    return !response.valid();
}

std::vector<Roadmap> client::get_roadmaps() const
{
    GetRoadmapsRequest request{};
    GetRoadmapsResponse response{};
    grpc::ClientContext context{};
    request.set_allocated_user(m_config->get_user().release());
    m_stub->GetRoadmaps(&context, request, &response);
    return std::vector<Roadmap>{response.roadmap().begin(), response.roadmap().end()};
}
