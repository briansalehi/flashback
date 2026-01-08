#pragma once

#include <gmock/gmock.h>
#include <types.pb.h>
#include <flashback/database.hpp>

namespace flashback
{
class mock_database final: public flashback::basic_database
{
public:
    // users
    MOCK_METHOD(bool, create_session, (uint64_t, std::string_view, std::string_view), (override));
    MOCK_METHOD(uint64_t, create_user, (std::string_view, std::string_view, std::string_view), (override));
    MOCK_METHOD(void, reset_password, (uint64_t, std::string_view), (override));
    MOCK_METHOD(bool, user_exists, (std::string_view), (override));
    MOCK_METHOD(std::unique_ptr<User>, get_user, (std::string_view), (override));
    MOCK_METHOD(std::unique_ptr<User>, get_user, (std::string_view, std::string_view), (override));
    MOCK_METHOD(void, revoke_session, (uint64_t, std::string_view), (override));
    MOCK_METHOD(void, revoke_sessions_except, (uint64_t, std::string_view), (override));

    // roadmaps
    MOCK_METHOD(uint64_t, create_roadmap, (std::string_view), (override));
    MOCK_METHOD(void, assign_roadmap, (uint64_t, uint64_t), (override));
    MOCK_METHOD(std::vector<flashback::Roadmap>, get_roadmaps, (uint64_t), (override));
    MOCK_METHOD(void, rename_roadmap, (uint64_t, std::string_view), (override));
    MOCK_METHOD(void, remove_roadmap, (uint64_t), (override));
    MOCK_METHOD(std::vector<Roadmap>, search_roadmaps, (std::string_view), (override));

    // subjects
    MOCK_METHOD(Subject, create_subject, (std::string), (override));
    MOCK_METHOD((std::map<uint64_t, Subject>), search_subjects, (std::string), (override));
    MOCK_METHOD(void, rename_subject, (uint64_t, std::string), (override));
};
} // flashback
