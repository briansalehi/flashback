#include <memory>
#include <gtest/gtest.h>
#include <flashback/mock_database.hpp>
#include <flashback/server.hpp>

class ServerTest: public testing::Test
{
public:
    ServerTest()
        :m_server{nullptr}
    {
    }

    void SetUp() override
    {
    }

private:
    std::shared_ptr<flashback::server> m_server;
    std::shared_ptr<flashback::mock_database> m_mock_database;
};

TEST_F(ServerTest, Construction)
{
    // m_server = std::make_shared<flashback::server>(m_mock_database);
}
