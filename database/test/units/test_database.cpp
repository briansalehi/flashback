#include <memory>
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
        m_user_id = create_user();
    }

    void TearDown() override
    {
        remove_users();
    }

    [[nodiscard]] uint64_t create_user()
    {
        EXPECT_NO_THROW(m_user_id = m_database->create_user(m_user_name, m_email, m_hash));
        EXPECT_GT(m_user_id, 0);

        return m_user_id;
    }

    void remove_users()
    {
        pqxx::work remove_user{*m_connection};
        remove_user.exec("delete from users");
        remove_user.commit();
    }

    std::unique_ptr<pqxx::connection> m_connection{nullptr};
    std::shared_ptr<flashback::database> m_database{nullptr};
    uint64_t m_user_id{};
    std::string m_user_name{"Flashback Test"};
    std::string m_email{"test@flashback.eu.com"};
    std::string m_hash{"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"};
    std::string m_token{"88888888888888888888888888888888"};
    std::string m_device_id{"0000000000000000"};

};

TEST_F(test_database, CreateDuplicateUser)
{
    uint64_t user_id{};
    EXPECT_NO_THROW(user_id = m_database->create_user(m_user_name, m_email, m_hash));
    ASSERT_EQ(user_id, 0);
}

TEST_F(test_database, CreateSession)
{
    auto [success, reason] = m_database->create_session(m_user_id, m_token, m_device_id);

    ASSERT_TRUE(success);
}

TEST_F(test_database, GetUser)
{
    std::unique_ptr<flashback::User> user{nullptr};

    user = m_database->get_user(m_email);

    ASSERT_NE(user, nullptr);
    EXPECT_GT(user->id(), 0);
    EXPECT_EQ(user->name(), m_user_name);
    EXPECT_EQ(user->email(), m_email);
    EXPECT_EQ(user->hash(), m_hash);
    EXPECT_TRUE(user->token().empty());
    EXPECT_TRUE(user->device().empty());
    EXPECT_TRUE(user->password().empty());

    auto [success, reason] = m_database->create_session(m_user_id, m_token, m_device_id);
    ASSERT_TRUE(success);

    user = m_database->get_user(m_email, m_device_id);

    ASSERT_NE(user, nullptr);
    EXPECT_GT(user->id(), 0);
    EXPECT_EQ(user->name(), m_user_name);
    EXPECT_EQ(user->email(), m_email);
    EXPECT_EQ(user->hash(), m_hash);
    EXPECT_EQ(user->token(), m_token);
    EXPECT_EQ(user->device(), m_device_id);
    EXPECT_TRUE(user->password().empty());
}

TEST_F(test_database, ResetPassword)
{
}
