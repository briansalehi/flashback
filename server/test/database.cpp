#include <gtest/gtest.h>
#include <flashback/database.hpp>

class DatabaseTest: public testing::Test
{
protected:
    explicit DatabaseTest()
        : m_database{}
    {
    }

    void SetUp()
    {
    }

private:
    flashback::database m_database;
};

TEST_F(DatabaseTest, Construction)
{
}