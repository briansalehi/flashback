#include <memory>
#include <string>
#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <flashback/mock_database.hpp>
#include <flashback/exception.hpp>
#include <flashback/server.hpp>

class test_server : public testing::Test
{
public:
    void SetUp() override
    {
        m_mock_database = std::make_shared<flashback::mock_database>();
        m_server = std::make_shared<flashback::server>(m_mock_database);
        m_user = std::make_shared<flashback::User>();
        m_user->set_email("user@flashback.eu.com");
        m_user->set_name("Flashback Test User");
        m_user->set_password(R"(strong password)");
        m_user->set_device(R"(aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee)");
        //m_user->set_id(1);
        //m_user->set_hash(R"($argon2id$v=19$m=262144,t=3,p=1$faiJerPBCLb2TEdTbGv8BQ$M0j9j6ojyIjD9yZ4+lBBNR/WAiWpImUcEcUhCL3u9gc)");
        //m_user->set_token(R"(iNFzgSaY2W+q42gM9lNVbB13v0odiLy6WnHbInbuvvE)");
    }

protected:
    std::shared_ptr<flashback::server> m_server{nullptr};
    std::shared_ptr<flashback::mock_database> m_mock_database{nullptr};
    std::shared_ptr<flashback::User> m_user{nullptr};
};

TEST_F(test_server, SignUpNonExistingUser)
{
    EXPECT_CALL(*m_mock_database, user_exists(m_user->email())).Times(1).WillOnce(testing::Return(false));
    EXPECT_CALL(*m_mock_database, create_user(m_user->name(), m_user->email(), testing::_)).Times(1).WillOnce(testing::Return(1));

    auto context{std::make_unique<grpc::ServerContext>()};
    auto request{std::make_unique<flashback::SignUpRequest>()};
    auto response{std::make_unique<flashback::SignUpResponse>()};
    auto user{std::make_unique<flashback::User>(*m_user)};
    grpc::Status status{};

    EXPECT_EQ(user->id(), 0);
    EXPECT_TRUE(user->hash().empty());
    EXPECT_FALSE(user->name().empty());
    EXPECT_FALSE(user->email().empty());
    EXPECT_FALSE(user->device().empty());
    EXPECT_FALSE(user->password().empty());

    request->set_allocated_user(user.release());

    EXPECT_NO_THROW(status = m_server->SignUp(context.get(), request.get(), response.get()));
    EXPECT_TRUE(status.ok());
    EXPECT_TRUE(response->success());
    ASSERT_TRUE(response->has_user());
    EXPECT_GT(response->user().id(), 0);
    EXPECT_FALSE(response->user().hash().empty());
    EXPECT_FALSE(response->user().name().empty());
    EXPECT_FALSE(response->user().email().empty());
    EXPECT_FALSE(response->user().device().empty());
    EXPECT_TRUE(response->user().password().empty());
}

TEST_F(test_server, SignUpWithExistingUser)
{
    EXPECT_CALL(*m_mock_database, user_exists(m_user->email())).Times(1).WillOnce(testing::Return(true));
    EXPECT_CALL(*m_mock_database, create_user).Times(0);

    auto context{std::make_unique<grpc::ServerContext>()};
    auto request{std::make_unique<flashback::SignUpRequest>()};
    auto response{std::make_unique<flashback::SignUpResponse>()};
    auto user{std::make_unique<flashback::User>(*m_user)};
    grpc::Status status{};

    EXPECT_EQ(user->id(), 0);
    EXPECT_TRUE(user->hash().empty());
    EXPECT_FALSE(user->name().empty());
    EXPECT_FALSE(user->email().empty());
    EXPECT_FALSE(user->device().empty());
    EXPECT_FALSE(user->password().empty());

    request->set_allocated_user(user.release());

    EXPECT_NO_THROW(status = m_server->SignUp(context.get(), request.get(), response.get()));
    EXPECT_TRUE(status.ok());
    EXPECT_FALSE(response->success());
    EXPECT_FALSE(response->has_user());
    EXPECT_EQ(response->user().id(), 0);
    EXPECT_TRUE(response->user().password().empty());
    EXPECT_TRUE(response->user().name().empty());
    EXPECT_TRUE(response->user().email().empty());
    EXPECT_TRUE(response->user().device().empty());
    EXPECT_TRUE(response->user().hash().empty());
}

TEST_F(test_server, SignUpWithIncompleteCredentials)
{
    EXPECT_CALL(*m_mock_database, user_exists(m_user->email())).Times(1).WillOnce(testing::Return(true));
    EXPECT_CALL(*m_mock_database, create_user).Times(0);

    auto context{std::make_unique<grpc::ServerContext>()};
    auto request{std::make_unique<flashback::SignUpRequest>()};
    auto response{std::make_unique<flashback::SignUpResponse>()};
    auto user{std::make_unique<flashback::User>()};
    grpc::Status status{};

    EXPECT_FALSE(response->has_user());
    EXPECT_EQ(user->id(), 0);
    EXPECT_TRUE(user->hash().empty());
    EXPECT_TRUE(user->name().empty());
    EXPECT_TRUE(user->email().empty());
    EXPECT_TRUE(user->device().empty());
    EXPECT_TRUE(user->password().empty());

    // sign up with an undefined user should fail
    EXPECT_NO_THROW(status = m_server->SignUp(context.get(), request.get(), response.get()));
    EXPECT_TRUE(status.ok());
    EXPECT_FALSE(response->success());
    EXPECT_FALSE(response->has_user());

    // sign up with a user without a name should fail
    request->set_allocated_user(user.release());
    EXPECT_NO_THROW(status = m_server->SignUp(context.get(), request.get(), response.get()));
    EXPECT_TRUE(status.ok());
    EXPECT_FALSE(response->success());
    EXPECT_FALSE(response->has_user());

    // sign up with a user without email should fail
    user = std::make_unique<flashback::User>();
    user->set_name(m_user->name());
    request->set_allocated_user(user.release());
    EXPECT_NO_THROW(status = m_server->SignUp(context.get(), request.get(), response.get()));
    EXPECT_TRUE(status.ok());
    EXPECT_FALSE(response->success());
    EXPECT_FALSE(response->has_user());

    // sign up with a user without device should fail
    user = std::make_unique<flashback::User>();
    user->set_name(m_user->name());
    user->set_email(m_user->email());
    request->set_allocated_user(user.release());
    EXPECT_NO_THROW(status = m_server->SignUp(context.get(), request.get(), response.get()));
    EXPECT_TRUE(status.ok());
    EXPECT_FALSE(response->success());
    EXPECT_FALSE(response->has_user());

    // sign up with a user without password should fail
    user = std::make_unique<flashback::User>();
    user->set_name(m_user->name());
    user->set_email(m_user->email());
    user->set_device(m_user->device());
    request->set_allocated_user(user.release());
    EXPECT_NO_THROW(status = m_server->SignUp(context.get(), request.get(), response.get()));
    EXPECT_TRUE(status.ok());
    EXPECT_FALSE(response->success());
    EXPECT_FALSE(response->has_user());

    // sign up a user that already exists should fail
    user = std::make_unique<flashback::User>();
    user->set_name(m_user->name());
    user->set_email(m_user->email());
    user->set_device(m_user->device());
    user->set_password(m_user->password());
    request->set_allocated_user(user.release());
    EXPECT_NO_THROW(m_server->SignUp(context.get(), request.get(), response.get()));
    EXPECT_TRUE(status.ok());
    EXPECT_FALSE(response->success());
    EXPECT_FALSE(response->has_user());
}
