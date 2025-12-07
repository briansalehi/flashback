#include <memory>
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
        m_connection = std::make_unique<pqxx::connection>("postgres://flashback@localhost:5432/flashback_template");
    }

    void TearDown() override
    {
    }

private:
    std::unique_ptr<pqxx::connection> m_connection;
    std::shared_ptr<flashback::database> m_database;
};

TEST_F(test_database, Construction)
{
}
