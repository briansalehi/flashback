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
    MOCK_METHOD(bool, create_session, (uint64_t, std::string_view, std::string_view), (const, override));
    MOCK_METHOD(uint64_t, create_user, (std::string_view, std::string_view, std::string_view), (const, override));
    MOCK_METHOD(void, reset_password, (uint64_t, std::string_view), (const, override));
    MOCK_METHOD(bool, user_exists, (std::string_view), (const, override));
    MOCK_METHOD(std::unique_ptr<User>, get_user, (std::string_view), (const, override));
    MOCK_METHOD(std::unique_ptr<User>, get_user, (std::string_view, std::string_view), (const, override));
    MOCK_METHOD(void, revoke_session, (uint64_t, std::string_view), (const, override));
    MOCK_METHOD(void, revoke_sessions_except, (uint64_t, std::string_view), (const, override));

    // roadmaps
    MOCK_METHOD(Roadmap, create_roadmap, (uint64_t, std::string), (const, override));
    MOCK_METHOD(std::vector<flashback::Roadmap>, get_roadmaps, (uint64_t), (const, override));
    MOCK_METHOD(void, rename_roadmap, (uint64_t, std::string_view), (const, override));
    MOCK_METHOD(void, remove_roadmap, (uint64_t), (const, override));
    MOCK_METHOD(std::vector<Roadmap>, search_roadmaps, (std::string_view), (const, override));
    MOCK_METHOD(Roadmap, clone_roadmap, (uint64_t, uint64_t), (const, override));

    // subjects
    MOCK_METHOD(Subject, create_subject, (std::string), (const, override));
    MOCK_METHOD((std::map<uint64_t, Subject>), search_subjects, (std::string), (const, override));
    MOCK_METHOD(void, rename_subject, (uint64_t, std::string), (const, override));
    MOCK_METHOD(void, remove_subject, (uint64_t), (const, override));
    MOCK_METHOD(void, merge_subjects, (uint64_t, uint64_t), (const, override));

    // milestones
    MOCK_METHOD(Milestone, add_milestone, (uint64_t, expertise_level, uint64_t), (const, override));
    MOCK_METHOD(Milestone, add_milestone, (uint64_t, expertise_level, uint64_t, uint64_t), (const, override));
    MOCK_METHOD(std::vector<Milestone>, get_milestones, (uint64_t), (const, override));
    MOCK_METHOD(void, add_requirement, (uint64_t, Milestone, Milestone), (const, override));
    MOCK_METHOD(std::vector<Milestone>, get_requirements, (uint64_t, uint64_t, expertise_level), (const, override));
    MOCK_METHOD(void, reorder_milestone, (uint64_t, uint64_t, uint64_t), (const, override));
    MOCK_METHOD(void, remove_milestone, (uint64_t, uint64_t), (const, override));
    MOCK_METHOD(void, change_milestone_level, (uint64_t, uint64_t, expertise_level), (const, override));

    // practices
    MOCK_METHOD(expertise_level, get_user_cognitive_level, (uint64_t, uint64_t), (const, override));
};
} // flashback
