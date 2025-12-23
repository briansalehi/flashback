#pragma once

#include <memory>
#include <vector>
#include <string_view>
#include <types.pb.h>

namespace flashback
{
class basic_database
{
public:
    virtual ~basic_database() = default;

    // user handling
    virtual bool create_session(uint64_t user_id, std::string_view token, std::string_view device) = 0;
    [[nodiscard]] virtual uint64_t create_user(std::string_view name, std::string_view email, std::string_view hash) = 0;
    virtual void reset_password(uint64_t user_id, std::string_view hash) = 0;
    [[nodiscard]] virtual bool user_exists(std::string_view email) = 0;
    [[nodiscard]] virtual std::unique_ptr<User> get_user(std::string_view email) = 0;
    [[nodiscard]] virtual std::unique_ptr<User> get_user(std::string_view token, std::string_view device) = 0;
    virtual void revoke_session(uint64_t user_id, std::string_view token) = 0;
    virtual void revoke_sessions_except(uint64_t user_id, std::string_view token) = 0;
    //rename_user
    //change_user_email
    //verify_user
    //suspend_user
    //ban_user
    //unlock_user

    // roadmaps
    virtual uint64_t create_roadmap(std::string_view name) = 0;
    virtual void assign_roadmap_to_user(uint64_t user_id, uint64_t roadmap_id) = 0;
    [[nodiscard]] virtual std::vector<Roadmap> get_roadmaps(uint64_t user_id) = 0;
    virtual void rename_roadmap(uint64_t roadmap_id, std::string_view modified_name) = 0;
    virtual void remove_roadmap(uint64_t roadmap_id) = 0;
    [[nodiscard]] virtual std::vector<Roadmap> search_roadmaps(std::string_view token) = 0;

    // milestones
    //add_milestone
    //reorder_milestones
    //remove_milestone
    //change_milestone_level

    // subjects
    //create_subject
    //rename_subject
    //remove_subject
    //remove_subject_with_cards
    //merge_subjects
    //add_resource_to_subject
    //drop_resource_from_subject
    //search_subjects

    // topics
    //create_topic
    //get_topics
    //reorder_topics
    //remove_topic
    //remove_topic_with_cards
    //merge_topics
    //rename_topic
    //move_topic
    //search_topics

    // resources
    //create_resource
    //search_resources
    //get_resources
    //rename_resource
    //remove_resource
    //remove_resource_with_cards
    //merge_resources
    //change_resource_type
    //edit_resource_link
    //change_section_pattern
    //change_resource_condition

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
    //reorder_sections
    //merge_sections
    //rename_section
    //move_section
    //search_sections

    // cards
    //create_card
    //add_card_to_section
    //add_card_to_topic
    //reorder_section_cards
    //reorder_topic_cards
    //remove_card_from_section
    //remove_card_from_topic
    //edit_card_headline
    //remove_card
    //merge_cards
    //get_study_cards
    //get_practice_cards
    //search_cards

    // blocks
    //create_block
    //edit_block
    //remove_block
    //edit_block_extension
    //edit_block_type
    //edit_block_meta_data
    //reorder_blocks
    //merge_blocks
    //split_block
};
} // flashback
