#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <flashback/basic_client.hpp>

class mock_client: public flashback::basic_client
{
public:
    MOCK_METHOD(bool, signup, (std::string, std::string, std::string), (override));
};
