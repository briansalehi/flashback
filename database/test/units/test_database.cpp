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
    ASSERT_TRUE(m_database->create_session(m_user->id(), m_user->token(), m_user->device()));
}

TEST_F(test_database, CreateSessionForNonExistingUser)
{
    uint64_t non_existing_user{};
    ASSERT_FALSE(m_database->create_session(non_existing_user, m_user->token(), m_user->device()));
}

TEST_F(test_database, GetUserWithDevice)
{
    std::unique_ptr<flashback::User> user = m_database->get_user(m_user->id(), m_user->device());

    ASSERT_EQ(user, nullptr);

    std::string unknown_device{R"(33333333-3333-3333-3333-333333333333)"};
    user = m_database->get_user(m_user->id(), m_user->device());

    ASSERT_EQ(user, nullptr);
    ASSERT_TRUE(m_database->create_session(m_user->id(), m_user->token(), m_user->device()));

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

TEST_F(test_database, GetUserWithEmail)
{
    std::string non_existing_email{"non_existing_user@flashback.eu.com"};
    std::unique_ptr<flashback::User> user{m_database->get_user(non_existing_email)};

    ASSERT_EQ(user, nullptr);

    user = m_database->get_user(m_user->email());

    ASSERT_NE(user, nullptr);
    EXPECT_GT(user->id(), 0);
    EXPECT_EQ(user->name(), m_user->name());
    EXPECT_EQ(user->email(), m_user->email());
    EXPECT_EQ(user->hash(), m_user->hash());
    EXPECT_TRUE(user->token().empty());
    EXPECT_TRUE(user->device().empty());
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
    bool session1_success;
    bool session2_success;

    ASSERT_TRUE(m_database->create_session(m_user->id(), m_user->token(), m_user->device()));
    ASSERT_TRUE(m_database->create_session(m_user->id(), token1, device1));
    ASSERT_TRUE(m_database->create_session(m_user->id(), token2, device2));

    m_database->revoke_sessions_except(m_user->id(), m_user->token());

    user = m_database->get_user(m_user->id(), m_user->device());
    EXPECT_NE(user, nullptr);

    user = m_database->get_user(m_user->id(), device1);
    ASSERT_EQ(user, nullptr);

    user = m_database->get_user(m_user->id(), device2);
    EXPECT_EQ(user, nullptr);
}

TEST_F(test_database, RevokeSessionsWithNonExistingToken)
{
    std::string non_existing_token{R"(3333333333+q42gM9lNVbB13v0odiLy6WnHbInbuvvE)"};

    ASSERT_TRUE(m_database->create_session(m_user->id(), m_user->token(), m_user->device()));

    m_database->revoke_sessions_except(m_user->id(), non_existing_token);

    std::unique_ptr<flashback::User> user = m_database->get_user(m_user->id(), m_user->device());
    ASSERT_NE(user, nullptr);
}

TEST_F(test_database, RevokeSingleSession)
{
    std::unique_ptr<flashback::User> user{nullptr};
    std::string token{R"(1111111111+q42gM9lNVbB13v0odiLy6WnHbInbuvvE)"};
    std::string device{R"(11111111-1111-1111-1111-111111111111)"};

    ASSERT_TRUE(m_database->create_session(m_user->id(), m_user->token(), m_user->device()));
    ASSERT_TRUE(m_database->create_session(m_user->id(), token, device));

    m_database->revoke_session(m_user->id(), token);

    user = m_database->get_user(m_user->id(), device);
    ASSERT_EQ(user, nullptr);

    user = m_database->get_user(m_user->id(), m_user->device());
    EXPECT_NE(user, nullptr);
}

TEST_F(test_database, ResetPassword)
{
    std::unique_ptr<flashback::User> user{nullptr};
    std::string hash{R"($argon2id$v=19$m=262144,t=3,p=1$faiJerPACLb2TEdTbGv8AQ$M0j9j6ojyIjD9yZ4+lAANR/WAiWpImUcEcUhCL3u9gc)"};

    ASSERT_TRUE(m_database->create_session(m_user->id(), m_user->token(), m_user->device()));

    user = m_database->get_user(m_user->id(), m_user->device());

    ASSERT_NE(user, nullptr);
    EXPECT_EQ(user->hash(), m_user->hash());

    m_database->reset_password(m_user->id(), hash);

    user = m_database->get_user(m_user->id(), m_user->device());
    ASSERT_NE(user, nullptr);
    EXPECT_EQ(user->hash(), hash);
}

TEST_F(test_database, UserExists)
{
    EXPECT_FALSE(m_database->user_exists("non-existing-user@flashback.eu.com"));
    EXPECT_TRUE(m_database->user_exists(m_user->email()));
    EXPECT_TRUE(m_database->create_session(m_user->id(), m_user->token(), m_user->device()));
    EXPECT_TRUE(m_database->user_exists(m_user->email()));
}
