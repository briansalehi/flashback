#pragma once

#include <memory>
#include <vector>
#include <map>
#include <string>
#include <string_view>
#include <types.pb.h>

namespace flashback
{
class basic_database
{
public:
    virtual ~basic_database() = default;

    // user handling
    [[nodiscard]] virtual bool create_session(uint64_t user_id, std::string_view token, std::string_view device) const = 0;
    [[nodiscard]] virtual uint64_t create_user(std::string_view name, std::string_view email, std::string_view hash) const = 0;
    virtual void reset_password(uint64_t user_id, std::string_view hash) const = 0;
    [[nodiscard]] virtual bool user_exists(std::string_view email) const = 0;
    [[nodiscard]] virtual std::unique_ptr<User> get_user(std::string_view email) const = 0;
    [[nodiscard]] virtual std::unique_ptr<User> get_user(std::string_view token, std::string_view device) const = 0;
    virtual void revoke_session(uint64_t user_id, std::string_view token) const = 0;
    virtual void revoke_sessions_except(uint64_t user_id, std::string_view token) const = 0;
    //rename_user
    //change_user_email
    //verify_user
    //suspend_user
    //ban_user
    //unlock_user
    //user_is_qualified

    // roadmaps
    [[nodiscard]] virtual Roadmap create_roadmap(uint64_t user_id, std::string name) const = 0;
    [[nodiscard]] virtual std::vector<Roadmap> get_roadmaps(uint64_t user_id) const = 0;
    virtual void rename_roadmap(uint64_t roadmap_id, std::string_view modified_name) const = 0;
    virtual void remove_roadmap(uint64_t roadmap_id) const = 0;
    [[nodiscard]] virtual std::vector<Roadmap> search_roadmaps(std::string_view token) const = 0;
    [[nodiscard]] virtual Roadmap clone_roadmap(uint64_t user_id, uint64_t roadmap_id) const = 0;
    //get_roadmap_weight

    // milestones
    [[nodiscard]] virtual Milestone add_milestone(uint64_t subject_id, expertise_level subject_level, uint64_t roadmap_id) const = 0;
    [[nodiscard]] virtual Milestone add_milestone(uint64_t subject_id, expertise_level subject_level, uint64_t roadmap_id, uint64_t position) const = 0;
    [[nodiscard]] virtual std::vector<Milestone> get_milestones(uint64_t roadmap_id) const = 0;
    virtual void add_requirement(uint64_t roadmap_id, Milestone milestone, Milestone required_milestone) const = 0;
    [[nodiscard]] virtual std::vector<Milestone> get_requirements(uint64_t roadmap_id, uint64_t subject_id, expertise_level subject_level) const = 0;
    virtual void reorder_milestone(uint64_t roadmap_id, uint64_t current_position, uint64_t target_position) const = 0;
    virtual void remove_milestone(uint64_t roadmap_id, uint64_t subject_id) const = 0;
    virtual void change_milestone_level(uint64_t roadmap_id, uint64_t subject_id, expertise_level level) const = 0;

    // subjects
    [[nodiscard]] virtual Subject create_subject(std::string name) const = 0;
    virtual void rename_subject(uint64_t id, std::string name) const = 0;
    virtual void remove_subject(uint64_t id) const = 0;
    virtual void merge_subjects(uint64_t source, uint64_t target) const = 0;
    [[nodiscard]] virtual std::map<uint64_t, Subject> search_subjects(std::string name) const = 0;
    //get_subject_weight

    // topics
    //create_topic
    //get_topics
    //reorder_topic
    //remove_topic
    //merge_topics
    //rename_topic
    //move_topic
    //search_topics
    //change_topic_level
    //get_practice_topics
    //get_practice_mode
    //get_topic_weight

    // resources
    [[nodiscard]] virtual Resource create_resource(Resource const& resource) const = 0;
    virtual void add_resource_to_subject(uint64_t resource_id, uint64_t subject_id) const = 0;
    [[nodiscard]] virtual std::vector<Resource> get_resources(uint64_t subject_id) const = 0;
    virtual void drop_resource_from_subject(uint64_t resource_id, uint64_t subject_id) const = 0;
    //search_resources
    //rename_resource
    //remove_resource
    //merge_resources
    //change_resource_type
    //edit_resource_link
    //edit_section_pattern
    //edit_resource_production
    //edit_resource_expiration
    //get_relevant_subjects
    //get_resource_state
    //mark_resource_as_completed

    // providers
    //create_provider
    //search_provider
    //add_provider
    //rename_provider
    //remove_provider
    //merge_providers

    // presenters
    //create_presenter
    //search_presenter
    //add_presenter
    //rename_presenter
    //remove_presenter
    //merge_presenters

    // sections
    //create_section
    //get_sections
    //remove_section
    //remove_section_with_cards
    //reorder_section
    //merge_sections
    //rename_section
    //move_section
    //search_sections
    //get_section_state
    //mark_section_as_reviewed
    //mark_section_as_completed

    // cards
    //create_card
    //add_card_to_section
    //add_card_to_topic
    //edit_card_headline
    //remove_card
    //merge_cards
    //get_study_cards
    //get_practice_cards
    //get_topic_cards
    //get_section_cards
    //search_cards
    //move_card_to_section
    //move_card_to_topic
    //make_progress
    //mark_card_as_reviewed
    //mark_card_as_completed
    //get_variations
    //is_absolute
    //get_card_weight

    // blocks
    //create_block
    //get_blocks
    //edit_block
    //remove_block
    //reorder_block
    //merge_blocks
    //split_block
    //move_block

    // assessments
    //create_assessment
    //add_card_to_assessment
    //get_assessments
    //get_assessment_coverage
    //get_assimilation_coverage
    //expand_assessment
    //diminish_assessment
    [[nodiscard]] virtual expertise_level get_user_cognitive_level(uint64_t user_id, uint64_t subject_id) const = 0;

    // nerves
    //create_nerve

    // anomaly detection
    //get_duplicate_cards
    //get_lost_cards
    //get_out_of_shelves
    //get_unshelved_resources
    //get_unapproved_topics_cards

    // client
    //certify_client
    //get_client_certifications

    // triggers
    //section_is_reviewed
    //complete_cards
    //approve_cards
    //release_cards
};
} // flashback
