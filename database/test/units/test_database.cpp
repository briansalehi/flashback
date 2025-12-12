#include <memory>
#include <exception>
#include <filesystem>
#include <gtest/gtest.h>
#include <pqxx/pqxx>
#include <flashback/database.hpp>

class test_database: public testing::Test
{
protected:
    explicit test_database()
        : m_connection{nullptr}
        , m_database{nullptr}
    {
    }

    ~test_database() override = default;

    void SetUp() override
    {
        m_connection = std::make_unique<pqxx::connection>("postgres://flashback@localhost:5432/flashback_test");
        m_database = std::make_unique<flashback::database>("flashback_test");
    }

    void TearDown() override
    {
    }

    std::unique_ptr<pqxx::connection> m_connection;
    std::shared_ptr<flashback::database> m_database;
};

TEST_F(test_database, CreateUser)
{
    std::string name{"Flashback Test"};
    std::string email{"test@flashback.eu.com"};
    std::string hash{"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"};
    uint64_t user_id{};

    ASSERT_NO_THROW((user_id = m_database->create_user(name, email, hash)));
    ASSERT_GT(user_id, 0);

    pqxx::work remove_user{*m_connection};
    remove_user.exec("delete from users");
    remove_user.commit();

    //ASSERT_ANY_THROW(user_id = m_database->create_user(name, email, hash));
}
