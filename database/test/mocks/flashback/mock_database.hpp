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

    // resources
    MOCK_METHOD(Resource, create_resource, (Resource const&), (const, override));
    MOCK_METHOD(void, add_resource_to_subject, (uint64_t, uint64_t), (const, override));
    MOCK_METHOD(std::vector<Resource>, get_resources, (uint64_t), (const, override));
    MOCK_METHOD(void, drop_resource_from_subject, (uint64_t, uint64_t), (const, override));
    MOCK_METHOD((std::map<uint64_t, Resource>), search_resources, (std::string), (const, override));
    MOCK_METHOD(void, edit_resource_link, (uint64_t, std::string), (const, override));
    MOCK_METHOD(void, change_resource_type, (uint64_t, Resource::resource_type), (const, override));
    MOCK_METHOD(void, change_section_pattern, (uint64_t, Resource::section_pattern const pattern), (const, override));
    MOCK_METHOD(void, edit_resource_production, (uint64_t, uint64_t), (const, override));
    MOCK_METHOD(void, edit_resource_expiration, (uint64_t, uint64_t), (const, override));
    MOCK_METHOD(void, rename_resource, (uint64_t, std::string), (const, override));
    MOCK_METHOD(void, remove_resource, (uint64_t), (const, override));
    MOCK_METHOD(void, merge_resources, (uint64_t, uint64_t), (const, override));

    // providers
    MOCK_METHOD(Provider, create_provider, (std::string), (const, override));
    MOCK_METHOD(void, add_provider, (uint64_t, uint64_t), (const, override));
    MOCK_METHOD(void, drop_provider, (uint64_t, uint64_t), (const, override));
    MOCK_METHOD((std::map<uint64_t, Provider>), search_providers, (std::string), (const, override));
    MOCK_METHOD(void, rename_provider, (uint64_t, std::string), (const, override));
    MOCK_METHOD(void, remove_provider, (uint64_t), (const, override));
    MOCK_METHOD(void, merge_providers, (uint64_t, uint64_t), (const, override));

    // presenters
    MOCK_METHOD(Presenter, create_presenter, (std::string), (const, override));
    MOCK_METHOD(void, add_presenter, (uint64_t, uint64_t), (const, override));
    MOCK_METHOD(void, drop_presenter, (uint64_t, uint64_t), (const, override));
    MOCK_METHOD((std::map<uint64_t, Presenter>), search_presenters, (std::string), (const, override));
    MOCK_METHOD(void, rename_presenter, (uint64_t, std::string), (const, override));
    MOCK_METHOD(void, remove_presenter, (uint64_t), (const, override));
    MOCK_METHOD(void, merge_presenters, (uint64_t, uint64_t), (const, override));

    // nerves
    MOCK_METHOD(Resource, create_nerve, (uint64_t, std::string, uint64_t, uint64_t), (const, override));
    MOCK_METHOD(std::vector<Resource>, get_nerves, (uint64_t), (const, override));

    // practices
    MOCK_METHOD(expertise_level, get_user_cognitive_level, (uint64_t, uint64_t), (const, override));
};
} // flashback
