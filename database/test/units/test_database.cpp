#include <memory>
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

TEST_F(test_database, ResetPassword)
{
}
