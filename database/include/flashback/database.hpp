#pragma once

#include <pqxx/pqxx>
#include <server.grpc.pb.h>
#include <flashback/basic_database.hpp>

class test_database;

namespace flashback
{
class database: public basic_database
{
public:
    explicit database(std::string client, std::string name = "flashback", std::string address = "localhost", std::string port = "5432");
    database(database const& copy);
    database& operator=(database const& copy);
    database(database&& copy) noexcept;
    database& operator=(database&& copy) noexcept;
    ~database() override = default;

    // users
    [[nodiscard]] bool create_session(uint64_t user_id, std::string_view token, std::string_view device) const override;
    [[nodiscard]] uint64_t create_user(std::string_view name, std::string_view email, std::string_view hash) const override;
    void reset_password(uint64_t user_id, std::string_view hash) const override;
    [[nodiscard]] bool user_exists(std::string_view email) const override;
    [[nodiscard]] std::unique_ptr<User> get_user(std::string_view email) const override;
    [[nodiscard]] std::unique_ptr<User> get_user(std::string_view token, std::string_view device) const override;
    void revoke_session(uint64_t user_id, std::string_view token) const override;
    void revoke_sessions_except(uint64_t user_id, std::string_view token) const override;

    // roadmaps
    [[nodiscard]] Roadmap create_roadmap(uint64_t user_id, std::string name) const override;
    [[nodiscard]] std::vector<Roadmap> get_roadmaps(uint64_t user_id) const override;
    void rename_roadmap(uint64_t roadmap_id, std::string_view modified_name) const override;
    void remove_roadmap(uint64_t roadmap_id) const override;
    [[nodiscard]] std::map<uint64_t, Roadmap> search_roadmaps(std::string_view search_pattern) const override;

    // subjects
    [[nodiscard]] Subject create_subject(std::string name) const override;
    [[nodiscard]] std::map<uint64_t, Subject> search_subjects(std::string_view search_pattern) const override;
    void rename_subject(uint64_t subject_id, std::string name) const override;
    void remove_subject(uint64_t subject_id) const override;
    void merge_subjects(uint64_t source, uint64_t target) const override;

    // milestones
    [[nodiscard]] Milestone add_milestone(uint64_t subject_id, expertise_level subject_level, uint64_t roadmap_id) const override;
    [[nodiscard]] Milestone add_milestone(uint64_t subject_id, expertise_level subject_level, uint64_t roadmap_id, uint64_t position) const override;
    [[nodiscard]] std::vector<Milestone> get_milestones(uint64_t roadmap_id) const override;
    void add_requirement(uint64_t roadmap_id, Milestone milestone, Milestone required_milestone) const override;
    [[nodiscard]] std::vector<Milestone> get_requirements(uint64_t roadmap_id, uint64_t subject_id, expertise_level subject_level) const override;
    [[nodiscard]] Roadmap clone_roadmap(uint64_t user_id, uint64_t roadmap_id) const override;
    void reorder_milestone(uint64_t roadmap_id, uint64_t current_position, uint64_t target_position) const override;
    void remove_milestone(uint64_t roadmap_id, uint64_t subject_id) const override;
    void change_milestone_level(uint64_t roadmap_id, uint64_t subject_id, expertise_level level) const override;

    // resources
    [[nodiscard]] Resource create_resource(Resource const& resource) const override;
    void add_resource_to_subject(uint64_t resource_id, uint64_t subject_id) const override;
    [[nodiscard]] std::vector<Resource> get_resources(uint64_t user_id, uint64_t subject_id) const override;
    void drop_resource_from_subject(uint64_t resource_id, uint64_t subject_id) const override;
    [[nodiscard]] std::map<uint64_t, Resource> search_resources(std::string_view search_pattern) const override;
    void edit_resource_link(uint64_t resource_id, std::string link) const override;
    void change_resource_type(uint64_t resource_id, Resource::resource_type type) const override;
    void change_section_pattern(uint64_t resource_id, Resource::section_pattern pattern) const override;
    void edit_resource_production(uint64_t resource_id, uint64_t production) const override;
    void edit_resource_expiration(uint64_t resource_id, uint64_t expiration) const override;
    void rename_resource(uint64_t resource_id, std::string name) const override;
    void remove_resource(uint64_t resource_id) const override;
    void merge_resources(uint64_t source_id, uint64_t target_id) const override;

    // sections
    [[nodiscard]] Section create_section(uint64_t resource_id, uint64_t position, std::string name, std::string link) const override;
    [[nodiscard]] std::map<uint64_t, Section> get_sections(uint64_t resource_id) const override;
    void remove_section(uint64_t resource_id, uint64_t position) const override;
    void reorder_section(uint64_t resource_id, uint64_t current_position, uint64_t target_position) const override;
    void merge_sections(uint64_t resource_id, uint64_t source_position, uint64_t target_position) const override;
    void rename_section(uint64_t resource_id, uint64_t position, std::string name) const override;
    void move_section(uint64_t resource_id, uint64_t position, uint64_t target_resource_id, uint64_t target_position) const override;
    [[nodiscard]] std::map<uint64_t, Section> search_sections(uint64_t resource_id, std::string_view search_pattern) const override;
    void edit_section_link(uint64_t resource_id, uint64_t position, std::string link) const override;

    // topics
    [[nodiscard]] Topic create_topic(uint64_t subject_id, std::string name, expertise_level level, uint64_t position) const override;
    [[nodiscard]] std::map<uint64_t, Topic> get_topics(uint64_t subject_id, expertise_level level) const override;
    void reorder_topic(uint64_t subject_id, expertise_level level, uint64_t source_position, uint64_t target_position) const override;
    void remove_topic(uint64_t subject_id, expertise_level level, uint64_t position) const override;
    void merge_topics(uint64_t subject_id, expertise_level level, uint64_t source_position, uint64_t target_position) const override;
    void rename_topic(uint64_t subject_id, expertise_level level, uint64_t position, std::string name) const override;
    void move_topic(uint64_t subject_id, expertise_level level, uint64_t position, uint64_t target_subject_id, uint64_t target_position) const override;
    [[nodiscard]] std::map<uint64_t, Topic> search_topics(uint64_t subject_id, expertise_level level, std::string_view search_pattern) const override;
    void change_topic_level(uint64_t subject_id, uint64_t position, flashback::expertise_level level, flashback::expertise_level target) const override;

    // providers
    [[nodiscard]] Provider create_provider(std::string name) const override;
    void add_provider(uint64_t resource_id, uint64_t provider_id) const override;
    void drop_provider(uint64_t resource_id, uint64_t provider_id) const override;
    [[nodiscard]] std::map<uint64_t, Provider> search_providers(std::string_view search_pattern) const override;
    void rename_provider(uint64_t provider_id, std::string name) const override;
    void remove_provider(uint64_t provider_id) const override;
    void merge_providers(uint64_t source_id, uint64_t target_id) const override;

    // presenters
    [[nodiscard]] Presenter create_presenter(std::string name) const override;
    void add_presenter(uint64_t resource_id, uint64_t presenter_id) const override;
    void drop_presenter(uint64_t resource_id, uint64_t presenter_id) const override;
    [[nodiscard]] std::map<uint64_t, Presenter> search_presenters(std::string_view search_pattern) const override;
    void rename_presenter(uint64_t presenter_id, std::string name) const override;
    void remove_presenter(uint64_t presenter_id) const override;
    void merge_presenters(uint64_t source_id, uint64_t target_id) const override;

    // cards
    [[nodiscard]] Card create_card(Card card) const override;
    void add_card_to_section(uint64_t card_id, uint64_t resource_id, uint64_t section_position) const override;
    void add_card_to_topic(uint64_t card_id, uint64_t subject_id, uint64_t topic_position, expertise_level topic_level) const override;
    void edit_card_headline(uint64_t card_id, std::string headline) const override;
    void remove_card(uint64_t card_id) const override;
    void merge_cards(uint64_t source_id, uint64_t target_id, std::string) const override;
    [[nodiscard]] std::map<uint64_t, Card> search_cards(uint64_t subject_id, expertise_level level, std::string_view search_pattern) const override;
    void move_card_to_section(uint64_t card_id, uint64_t resource_id, uint64_t section_position, uint64_t target_section_position) const override;
    void move_card_to_topic(uint64_t card_id, uint64_t subject_id, uint64_t topic_position, expertise_level topic_level, uint64_t target_subject, uint64_t target_position,
                            expertise_level target_level) const override;
    [[nodiscard]] std::vector<Card> get_section_cards(uint64_t resource_id, uint64_t sections_position) const override;
    [[nodiscard]] std::vector<Card> get_topic_cards(uint64_t subject_id, uint64_t topic_position, expertise_level topic_level) const override;

    // blocks
    [[nodiscard]] Block create_block(uint64_t card_id, Block block) const override;
    [[nodiscard]] std::map<uint64_t, Block> get_blocks(uint64_t card_id) const override;
    void remove_block(uint64_t card_id, uint64_t block_position) const override;
    void edit_block_content(uint64_t card_id, uint64_t block_position, std::string content) const override;
    void change_block_type(uint64_t card_id, uint64_t block_position, Block::content_type type) const override;
    void edit_block_extension(uint64_t card_id, uint64_t block_position, std::string extension) const override;
    void edit_block_metadata(uint64_t card_id, uint64_t block_position, std::string metadata) const override;
    void reorder_block(uint64_t card_id, uint64_t block_position, uint64_t target_position) const override;
    void merge_blocks(uint64_t card_id, uint64_t source_position, uint64_t target_position) const override;
    [[nodiscard]] std::map<uint64_t, Block> split_block(uint64_t card_id, uint64_t block_position) const override;
    void move_block(uint64_t card_id, uint64_t block_position, uint64_t target_card_id, uint64_t target_position) const override;

    // nerves
    [[nodiscard]] Resource create_nerve(uint64_t user_id, std::string resource_name, uint64_t subject_id, uint64_t expiration) const override;
    [[nodiscard]] std::vector<Resource> get_nerves(uint64_t user_id) const override;

    // practices
    [[nodiscard]] expertise_level get_user_cognitive_level(uint64_t user_id, uint64_t roadmap_id, uint64_t subject_id) const override;
    [[nodiscard]] practice_mode get_practice_mode(uint64_t user_id, uint64_t subject_id, expertise_level level) const override;
    [[nodiscard]] std::vector<Topic> get_practice_topics(uint64_t user_id, uint64_t roadmap_id, uint64_t subject_id) const override;
    [[nodiscard]] std::vector<Card> get_practice_cards(uint64_t user_id, uint64_t roadmap_id, uint64_t subject_id, expertise_level level, uint64_t topic_position) const override;
    [[nodiscard]] closure_state get_resource_state(uint64_t resource_id) const override;
    void study(uint64_t user_id, uint64_t card_id, std::chrono::seconds duration) const override;
    [[nodiscard]] std::map<uint64_t, Resource> get_study_resources(uint64_t user_id) const override;
    std::map<uint64_t, Card> get_study_cards(uint64_t user_id, uint64_t resource_id, uint64_t section_position) const override;
    void mark_card_as_reviewed(uint64_t card_id) const override;
    void mark_card_as_completed(uint64_t card_id) const override;
    void mark_section_as_reviewed(uint64_t resource_id, uint64_t section_position) const override;
    void mark_section_as_completed(uint64_t resource_id, uint64_t section_position) const override;
    void mark_card_as_approved(uint64_t card_id) const override;
    void mark_card_as_released(uint64_t card_id) const override;
    void make_progress(uint64_t user_id, uint64_t card_id, uint64_t duration, practice_mode mode) const override;
    Weight get_progress_weight(uint64_t user_id) const override;
    std::vector<Card> get_variations(uint64_t card_id) const override;
    bool is_absolute(uint64_t card_id) const override;

    // assessments
    void create_assessment(uint64_t subject_id, expertise_level level, uint64_t topic_position, uint64_t card_id) const override;
    void get_topic_coverage(uint64_t assessment_id) const override;
    void get_assessment_coverage(uint64_t subject_id, uint64_t topic_position, expertise_level max_level) const override;
    void get_assimilation_coverage(uint64_t user_id, uint64_t assessment_id) const override;
    std::vector<Card> get_topic_assessments(uint64_t user_id, uint64_t subject_id, uint64_t topic_position, expertise_level max_level) const override;
    std::vector<Card> get_assessments(uint64_t user_id, uint64_t subject_id, uint64_t topic_position) const override;
    void expand_assessment(uint64_t assessment_id, uint64_t subject_id, expertise_level level, uint64_t topic_position) const override;
    void diminish_assessment(uint64_t assessment_id, uint64_t subject_id, expertise_level level, uint64_t topic_position) const override;

private:
    template <typename... Args>
    [[nodiscard]] pqxx::result query(std::string_view const format, Args&&... args) const
    {
        pqxx::work work{*m_connection};
        pqxx::result result{work.exec(format, pqxx::params{std::forward<Args>(args)...})};
        work.commit();
        return result;
    }

    template <typename... Args>
    void exec(std::string_view const format, Args&&... args) const
    {
        pqxx::work work{*m_connection};
        work.exec(format, pqxx::params{std::forward<Args>(args)...});
        work.commit();
    }

    void throw_back_progress(uint64_t user_id, uint64_t card_id, uint64_t days) const;

    static expertise_level to_level(std::string_view const level);
    static std::string level_to_string(expertise_level const level);
    static std::string resource_type_to_string(Resource::resource_type const type);
    static Resource::resource_type to_resource_type(std::string_view const type_string);
    static std::string section_pattern_to_string(Resource::section_pattern const pattern);
    static Resource::section_pattern to_section_pattern(std::string_view const pattern_string);
    static Card::card_state to_card_state(std::string_view const state_string);
    static std::string card_state_to_string(Card::card_state const state);
    static Block::content_type to_content_type(std::string_view const type_string);
    static std::string content_type_to_string(Block::content_type const type);
    static closure_state to_closure_state(std::string_view const state_string);
    static std::string closure_state_to_string(closure_state const state);
    static practice_mode to_practice_mode(std::string_view const mode_string);
    static std::string practice_mode_to_string(practice_mode const mode);

    std::unique_ptr<pqxx::connection> m_connection;
    friend class ::test_database;
};
} // flashback
