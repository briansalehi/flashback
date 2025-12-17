#include <memory>
#include <gtest/gtest.h>
#include <flashback/mock_database.hpp>
#include <flashback/server.hpp>

class test_server : public testing::Test
{
public:
    void SetUp() override
    {
        m_mock_database = std::make_shared<flashback::mock_database>();
        m_server = std::make_shared<flashback::server>(m_mock_database);
    }

private:
    std::shared_ptr<flashback::server> m_server{nullptr};
    std::shared_ptr<flashback::mock_database> m_mock_database{nullptr};
};

TEST_F(test_server, SignUp)
{
}
