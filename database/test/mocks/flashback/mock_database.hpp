#pragma once

#include <gmock/gmock.h>
#include <types.pb.h>
#include <flashback/database.hpp>

namespace flashback
{
class mock_database final : public flashback::basic_database
{
public:
    MOCK_METHOD(bool, create_session, (uint64_t, std::string_view, std::string_view), (override));
    MOCK_METHOD(uint64_t, create_user, (std::string_view, std::string_view, std::string_view), (override));
    MOCK_METHOD(void, reset_password, (uint64_t, std::string_view), (override));
    MOCK_METHOD(bool, user_exists, (std::string_view), (override));
    MOCK_METHOD(std::unique_ptr<User>, get_user, (std::string_view), (override));
    MOCK_METHOD(std::unique_ptr<User>, get_user, (uint64_t, std::string_view), (override));
    MOCK_METHOD(void, revoke_session, (uint64_t, std::string_view), (override));
    MOCK_METHOD(void, revoke_sessions_except, (uint64_t, std::string_view), (override));
    MOCK_METHOD(std::shared_ptr<flashback::GetRoadmapsResponse>, get_roadmaps, (uint64_t), (override));
};
} // flashback