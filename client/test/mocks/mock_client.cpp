#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <flashback/basic_client.hpp>

class mock_client: public flashback::basic_client
{
public:
    MOCK_METHOD(bool, signup, (std::string, std::string, std::string), (override));
    MOCK_METHOD(bool, signin, (std::string, std::string), (override));
    MOCK_METHOD(bool, needs_to_signup, (), (const, override));
    MOCK_METHOD(bool, needs_to_signin, (), (const, override));
    MOCK_METHOD(std::vector<flashback::Roadmap>, get_roadmaps, (), (const, override));
};
