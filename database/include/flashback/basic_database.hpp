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
    [[nodiscard]] virtual std::map<uint64_t, Roadmap> search_roadmaps(std::string_view search_pattern) const = 0;
    [[nodiscard]] virtual Roadmap clone_roadmap(uint64_t user_id, uint64_t roadmap_id) const = 0;

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
    virtual void rename_subject(uint64_t subject_id, std::string name) const = 0;
    virtual void remove_subject(uint64_t subject_id) const = 0;
    virtual void merge_subjects(uint64_t source, uint64_t target) const = 0;
    [[nodiscard]] virtual std::map<uint64_t, Subject> search_subjects(std::string_view search_pattern) const = 0;
    //add_alias
    //remove_alias

    // topics
    [[nodiscard]] virtual Topic create_topic(uint64_t subject_id, std::string name, flashback::expertise_level level, uint64_t position) const = 0;
    [[nodiscard]] virtual std::map<uint64_t, Topic> get_topics(uint64_t subject_id, expertise_level level) const = 0;
    virtual void reorder_topic(uint64_t subject_id, expertise_level level, uint64_t source_position, uint64_t target_position) const = 0;
    virtual void remove_topic(uint64_t subject_id, expertise_level level, uint64_t position) const = 0;
    virtual void merge_topics(uint64_t subject_id, expertise_level level, uint64_t source_position, uint64_t target_position) const = 0;
    virtual void rename_topic(uint64_t subject_id, expertise_level level, uint64_t position, std::string name) const = 0;
    virtual void move_topic(uint64_t subject_id, expertise_level level, uint64_t position, uint64_t target_subject_id, uint64_t target_position) const = 0;
    [[nodiscard]] virtual std::map<uint64_t, Topic> search_topics(uint64_t subject_id, expertise_level level, std::string_view search_pattern) const = 0;
    virtual void change_topic_level(uint64_t subject_id, uint64_t position, flashback::expertise_level level, flashback::expertise_level target) const = 0;

    // resources
    [[nodiscard]] virtual Resource create_resource(Resource const& resource) const = 0;
    virtual void add_resource_to_subject(uint64_t resource_id, uint64_t subject_id) const = 0;
    [[nodiscard]] virtual std::vector<Resource> get_resources(uint64_t subject_id) const = 0;
    virtual void drop_resource_from_subject(uint64_t resource_id, uint64_t subject_id) const = 0;
    [[nodiscard]] virtual std::map<uint64_t, Resource> search_resources(std::string_view search_pattern) const = 0;
    virtual void edit_resource_link(uint64_t resource_id, std::string link) const = 0;
    virtual void change_resource_type(uint64_t resource_id, Resource::resource_type type) const = 0;
    virtual void change_section_pattern(uint64_t resource_id, Resource::section_pattern pattern) const = 0;
    virtual void edit_resource_production(uint64_t resource_id, uint64_t production) const = 0;
    virtual void edit_resource_expiration(uint64_t resource_id, uint64_t expiration) const = 0;
    virtual void rename_resource(uint64_t resource_id, std::string name) const = 0;
    virtual void remove_resource(uint64_t resource_id) const = 0;
    virtual void merge_resources(uint64_t source_id, uint64_t target_id) const = 0;
    //get_relevant_subjects

    // providers
    [[nodiscard]] virtual Provider create_provider(std::string name) const = 0;
    virtual void add_provider(uint64_t resource_id, uint64_t provider_id) const = 0;
    virtual void drop_provider(uint64_t resource_id, uint64_t provider_id) const = 0;
    [[nodiscard]] virtual std::map<uint64_t, Provider> search_providers(std::string_view search_pattern) const = 0;
    virtual void rename_provider(uint64_t provider_id, std::string name) const = 0;
    virtual void remove_provider(uint64_t provider_id) const = 0;
    virtual void merge_providers(uint64_t source_id, uint64_t target_id) const =0;

    // presenters
    [[nodiscard]] virtual Presenter create_presenter(std::string name) const = 0;
    virtual void add_presenter(uint64_t resource_id, uint64_t presenter_id) const = 0;
    virtual void drop_presenter(uint64_t resource_id, uint64_t presenter_id) const = 0;
    [[nodiscard]] virtual std::map<uint64_t, Presenter> search_presenters(std::string_view search_pattern) const = 0;
    virtual void rename_presenter(uint64_t presenter_id, std::string name) const = 0;
    virtual void remove_presenter(uint64_t presenter_id) const = 0;
    virtual void merge_presenters(uint64_t source_id, uint64_t target_id) const = 0;

    // sections
    [[nodiscard]] virtual Section create_section(uint64_t resource_id, uint64_t position, std::string name, std::string link) const = 0;
    [[nodiscard]] virtual std::map<uint64_t, Section> get_sections(uint64_t resource_id) const = 0;
    virtual void remove_section(uint64_t resource_id, uint64_t position) const = 0;
    virtual void reorder_section(uint64_t resource_id, uint64_t current_position, uint64_t target_position) const = 0;
    virtual void merge_sections(uint64_t resource_id, uint64_t source_position, uint64_t target_position) const = 0;
    virtual void rename_section(uint64_t resource_id, uint64_t position, std::string name) const = 0;
    virtual void move_section(uint64_t resource_id, uint64_t position, uint64_t target_resource_id, uint64_t target_position) const = 0;
    [[nodiscard]] virtual std::map<uint64_t, Section> search_sections(uint64_t resource_id, std::string_view search_pattern) const = 0;
    virtual void edit_section_link(uint64_t resource_id, uint64_t position, std::string link) const = 0;
    //remove_section_with_cards

    // cards
    [[nodiscard]] virtual Card create_card(Card card) const = 0;
    virtual void add_card_to_section(uint64_t card_id, uint64_t resource_id, uint64_t section_position) const = 0;
    virtual void add_card_to_topic(uint64_t card_id, uint64_t subject_id, uint64_t topic_position, expertise_level topic_level) const = 0;
    virtual void edit_card_headline(uint64_t card_id, std::string headline) const = 0;
    virtual void remove_card(uint64_t card_id) const = 0;
    virtual void merge_cards(uint64_t source_id, uint64_t target_id, std::string headline) const = 0;
    [[nodiscard]] virtual std::map<uint64_t, Card> search_cards(uint64_t subject_id, expertise_level level, std::string_view search_pattern) const = 0;
    virtual void move_card_to_section(uint64_t card_id, uint64_t resource_id, uint64_t section_position, uint64_t target_section_position) const = 0;
    virtual void move_card_to_topic(uint64_t card_id, uint64_t subject_id, uint64_t topic_position, expertise_level topic_level, uint64_t target_subject, uint64_t target_position,
                                    expertise_level targe_level) const = 0;
    [[nodiscard]] virtual std::vector<Card> get_section_cards(uint64_t resource_id, uint64_t sections_position) const = 0;
    [[nodiscard]] virtual std::vector<Card> get_topic_cards(uint64_t subject_id, uint64_t topic_position, expertise_level topic_level) const = 0;

    // blocks
    [[nodiscard]] virtual Block create_block(uint64_t card_id, Block block) const = 0;
    [[nodiscard]] virtual std::map<uint64_t, Block> get_blocks(uint64_t card_id) const = 0;
    virtual void remove_block(uint64_t card_id, uint64_t block_position) const = 0;
    virtual void edit_block_content(uint64_t card_id, uint64_t block_position, std::string content) const = 0;
    virtual void change_block_type(uint64_t card_id, uint64_t block_position, Block::content_type type) const = 0;
    virtual void edit_block_extension(uint64_t card_id, uint64_t block_position, std::string extension) const = 0;
    virtual void edit_block_metadata(uint64_t card_id, uint64_t block_position, std::string metadata) const = 0;
    virtual void reorder_block(uint64_t card_id, uint64_t block_position, uint64_t target_position) const = 0;
    virtual void merge_blocks(uint64_t card_id, uint64_t source_position, uint64_t target_position) const = 0;
    [[nodiscard]] virtual std::map<uint64_t, Block> split_block(uint64_t card_id, uint64_t block_position) const = 0;
    virtual void move_block(uint64_t card_id, uint64_t block_position, uint64_t target_card_id, uint64_t target_position) const = 0;

    // progress
    [[nodiscard]] virtual expertise_level get_user_cognitive_level(uint64_t user_id, uint64_t subject_id) const = 0;
    virtual practice_mode get_practice_mode(uint64_t user_id, uint64_t subject_id, expertise_level level) const = 0;
    virtual std::vector<Topic> get_practice_topics(uint64_t user_id, uint64_t subject_id) const = 0;
    virtual std::vector<Card> get_practice_cards(uint64_t user_id, uint64_t subject_id, uint64_t topic_position) const = 0;
    virtual std::vector<Resource> get_study_resources(uint64_t user_id) const = 0;
    virtual std::map<uint64_t, Section> get_study_sections(uint64_t user_id, uint64_t resource_id) const = 0;
    virtual std::map<uint64_t, Card> get_study_cards(uint64_t user_id, uint64_t resource_id, uint64_t section_position) const = 0;
    virtual void mark_card_as_reviewed(uint64_t card_id) const = 0;
    virtual void mark_card_as_completed(uint64_t card_id) const = 0;
    virtual void mark_section_as_reviewed(uint64_t resource_id, uint64_t section_position) const = 0;
    virtual void mark_section_as_completed(uint64_t resource_id, uint64_t section_position) const = 0;
    virtual void mark_card_as_approved(uint64_t card_id) const = 0;
    virtual void mark_card_as_released(uint64_t card_id) const = 0;
    virtual void make_progress(uint64_t user_id, uint64_t card_id, uint64_t duration, practice_mode mode) const = 0;
    virtual closure_state get_section_state(uint64_t resource_id, uint64_t section_position) const = 0;
    virtual closure_state get_resource_state(uint64_t resource_id) const = 0;
    virtual Weight get_progress_weight(uint64_t user_id) const = 0;
    virtual std::vector<Card> get_variations(uint64_t card_id) const = 0;
    virtual bool is_absolute(uint64_t card_id) const = 0;

    // assessments
    virtual void create_assessment(uint64_t subject_id, expertise_level level, uint64_t topic_position, uint64_t card_id) const = 0;
    virtual void get_topic_coverage(uint64_t assessment_id) const = 0;
    virtual void get_assessment_coverage(uint64_t subject_id, uint64_t topic_position, expertise_level max_level) const = 0;
    virtual void get_assimilation_coverage(uint64_t user_id, uint64_t assessment_id) const = 0;
    virtual std::vector<Card> get_topic_assessments(uint64_t user_id, uint64_t subject_id, uint64_t topic_position, expertise_level max_level) const = 0;
    virtual std::vector<Card> get_assessments(uint64_t user_id, uint64_t subject_id, uint64_t topic_position) const = 0;
    virtual void expand_assessment(uint64_t assessment_id, uint64_t subject_id, expertise_level level, uint64_t topic_position) const = 0;
    virtual void diminish_assessment(uint64_t assessment_id, uint64_t subject_id, expertise_level level, uint64_t topic_position) const = 0;

    // nerves
    [[nodiscard]] virtual Resource create_nerve(uint64_t user_id, std::string resource_name, uint64_t subject_id, uint64_t expiration) const = 0;
    [[nodiscard]] virtual std::vector<Resource> get_nerves(uint64_t user_id) const = 0;

    // anomaly detection
    //get_duplicate_cards
    //get_lost_cards
    //get_out_of_shelves
    //get_unshelved_resources
    //get_unapproved_topic_cards

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
