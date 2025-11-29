#pragma once

#include <gmock/gmock.h>
#include <types.pb.h>
#include <flashback/database.hpp>

namespace flashback
{
class mock_database: public flashback::basic_database
{
public:
    MOCK_METHOD(std::shared_ptr<flashback::Roadmaps>, get_roadmaps, (uint64_t), (override));
    MOCK_METHOD((std::pair<bool, std::string>), create_session, (uint64_t, std::string_view, std::string_view), (override));
    MOCK_METHOD(uint64_t, create_user, (std::string_view, std::string_view, std::string_view), (override));
    MOCK_METHOD(void, reset_password, (uint64_t, std::string_view), (override));
    MOCK_METHOD(std::unique_ptr<flashback::User>, get_user, (std::string_view), (override));
    MOCK_METHOD(std::unique_ptr<flashback::User>, get_user, (std::string_view, std::string_view), (override));
};
} // flashback