#include <flashback/client.hpp>

using namespace flashback;

client::client(std::string address, std::string port)
    : m_channel{grpc::CreateChannel(std::format("{}:{}", address, port), grpc::InsecureChannelCredentials())}
    , m_stub{Server::NewStub(m_channel)}
{
}

bool client::signup(std::string name, std::string email, std::string password)
{
    flashback::SignUpRequest request{};
    flashback::SignUpResponse response{};

    auto user{std::make_unique<flashback::User>()};
    user->set_name(std::move(name));
    user->set_email(std::move(email));
    user->set_password(std::move(password));

    request.set_allocated_user(user.release());

    m_stub->SignUp(m_client_context.get(), request, &response);

    return response.success();
}
