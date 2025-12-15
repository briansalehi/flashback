#include <memory>
#include <utility>
#include <exception>
#include <filesystem>
#include <gtest/gtest.h>
#include <pqxx/pqxx>
#include <flashback/database.hpp>

class test_database: public testing::Test
{
protected:
    test_database()
    {
    }

    void SetUp() override
    {
        m_connection = std::make_unique<pqxx::connection>("postgres://flashback@localhost:5432/flashback_test");
        m_database = std::make_unique<flashback::database>("flashback_test");
        m_user = std::make_unique<flashback::User>();
        m_user->set_name("Flashback Test User");
        m_user->set_email("test@flashback.eu.com");
        m_user->set_hash(R"($argon2id$v=19$m=262144,t=3,p=1$faiJerPBCLb2TEdTbGv8BQ$M0j9j6ojyIjD9yZ4+lBBNR/WAiWpImUcEcUhCL3u9gc)");
        m_user->set_token(R"(iNFzgSaY2W+q42gM9lNVbB13v0odiLy6WnHbInbuvvE)");
        m_user->set_device(R"(aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee)");
        m_user->set_id(create_user());
    }

    void TearDown() override
    {
        remove_users();
    }

    uint64_t create_user()
    {
        uint64_t user_id{};

        EXPECT_NO_THROW(user_id = m_database->create_user(m_user->name(), m_user->email(), m_user->hash()));
        EXPECT_GT(user_id, 0);

        return user_id;
    }

    void remove_users()
    {
        pqxx::work remove_user{*m_connection};
        remove_user.exec("delete from users");
        remove_user.commit();
    }

    std::unique_ptr<pqxx::connection> m_connection{nullptr};
    std::shared_ptr<flashback::database> m_database{nullptr};
    std::unique_ptr<flashback::User> m_user{nullptr};
};

TEST_F(test_database, CreateDuplicateUser)
{
    uint64_t user_id{};
    EXPECT_NO_THROW(user_id = m_database->create_user(m_user->name(), m_user->email(), m_user->hash()));
    ASSERT_EQ(user_id, 0);
}

TEST_F(test_database, CreateSession)
{
    auto [success, reason] = m_database->create_session(m_user->id(), m_user->token(), m_user->device());

    ASSERT_TRUE(success);
}

TEST_F(test_database, GetUser)
{
    std::unique_ptr<flashback::User> user = m_database->get_user(m_user->id(), m_user->device());

    ASSERT_EQ(user, nullptr);

    std::string unknown_device{R"(33333333-3333-3333-3333-333333333333)"};
    user = m_database->get_user(m_user->id(), m_user->device());

    ASSERT_EQ(user, nullptr);

    auto [success, reason] = m_database->create_session(m_user->id(), m_user->token(), m_user->device());
    ASSERT_TRUE(success);

    user = m_database->get_user(m_user->id(), m_user->device());

    ASSERT_NE(user, nullptr);
    EXPECT_GT(user->id(), 0);
    EXPECT_EQ(user->name(), m_user->name());
    EXPECT_EQ(user->email(), m_user->email());
    EXPECT_EQ(user->hash(), m_user->hash());
    EXPECT_EQ(user->token(), m_user->token());
    EXPECT_EQ(user->device(), m_user->device());
    EXPECT_TRUE(user->password().empty());
    EXPECT_FALSE(user->verified());
}

TEST_F(test_database, RevokeSessionsExceptSelectedToken)
{
    std::unique_ptr<flashback::User> user{nullptr};
    std::string token1{R"(1111111111+q42gM9lNVbB13v0odiLy6WnHbInbuvvE)"};
    std::string token2{R"(2222222222+q42gM9lNVbB13v0odiLy6WnHbInbuvvE)"};
    std::string device1{R"(11111111-1111-1111-1111-111111111111)"};
    std::string device2{R"(22222222-2222-2222-2222-222222222222)"};
    bool session0_success;
    bool session1_success;
    bool session2_success;

    std::tie(session0_success, std::ignore) = m_database->create_session(m_user->id(), m_user->token(), m_user->device());
    ASSERT_TRUE(session0_success);

    std::tie(session1_success, std::ignore) = m_database->create_session(m_user->id(), token1, device1);
    ASSERT_TRUE(session1_success);

    std::tie(session2_success, std::ignore) = m_database->create_session(m_user->id(), token2, device2);
    ASSERT_TRUE(session2_success);

    m_database->revoke_sessions_except(m_user->id(), token1);

    user = m_database->get_user(m_user->id(), m_user->device());
    EXPECT_EQ(user, nullptr);

    user = m_database->get_user(m_user->id(), device1);
    ASSERT_NE(user, nullptr);
    EXPECT_GT(user->id(), 0);
    EXPECT_EQ(user->name(), m_user->name());
    EXPECT_EQ(user->email(), m_user->email());
    EXPECT_EQ(user->hash(), m_user->hash());
    EXPECT_EQ(user->token(), token1);
    EXPECT_EQ(user->device(), device1);
    EXPECT_TRUE(user->password().empty());
    EXPECT_FALSE(user->verified());

    user = m_database->get_user(m_user->id(), device2);
    EXPECT_EQ(user, nullptr);
}

TEST_F(test_database, RevokeSessionsWithNonExistingToken)
{
    std::string non_existing_token{R"(3333333333+q42gM9lNVbB13v0odiLy6WnHbInbuvvE)"};

    auto [success, reason] = m_database->create_session(m_user->id(), m_user->token(), m_user->device());
    ASSERT_TRUE(success);

    m_database->revoke_sessions_except(m_user->id(), non_existing_token);
    std::unique_ptr<flashback::User> user = m_database->get_user(m_user->id(), m_user->device());

    ASSERT_NE(user, nullptr);
    EXPECT_GT(user->id(), 0);
    EXPECT_EQ(user->name(), m_user->name());
    EXPECT_EQ(user->email(), m_user->email());
    EXPECT_EQ(user->hash(), m_user->hash());
    EXPECT_EQ(user->token(), m_user->token());
    EXPECT_EQ(user->device(), m_user->device());
    EXPECT_TRUE(user->password().empty());
    EXPECT_FALSE(user->verified());
}

TEST_F(test_database, ResetPassword)
{
}