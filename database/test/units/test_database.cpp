#include <memory>
#include <filesystem>
#include <gtest/gtest.h>
#include <flashback/database.hpp>

class test_database: public testing::Test
{
protected:
    explicit test_database()
        : m_database{nullptr}
    {
    }

    void SetUp()
    {
    }

private:
    std::shared_ptr<flashback::database> m_database;
};

TEST_F(test_database, Construction)
{
}
