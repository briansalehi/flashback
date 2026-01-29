#pragma once

#include <pqxx/pqxx>
#include <server.grpc.pb.h>
#include <flashback/basic_database.hpp>

namespace flashback
{
class database: public basic_database
{
public:
    explicit database(std::string client, std::string name = "flashback", std::string address = "localhost", std::string port = "5432");
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
    void rename_subject(uint64_t id, std::string name) const override;
    void remove_subject(uint64_t id) const override;
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
    [[nodiscard]] std::vector<Resource> get_resources(uint64_t subject_id) const override;
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
    void move_card_to_topic(uint64_t card_id, uint64_t subject_id, uint64_t topic_position, expertise_level topic_level, uint64_t target_subject, uint64_t target_position, expertise_level target_level) const override;
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
    [[nodiscard]] std::pair<Block, Block> split_block(uint64_t card_id, uint64_t block_position) const override;
    void move_block(uint64_t card_id, uint64_t block_position, uint64_t target_card_id, uint64_t target_position) const override;

    // nerves
    [[nodiscard]] Resource create_nerve(uint64_t user_id, std::string resource_name, uint64_t subject_id, uint64_t expiration) const override;
    [[nodiscard]] std::vector<Resource> get_nerves(uint64_t user_id) const override;

    // practices
    [[nodiscard]] expertise_level get_user_cognitive_level(uint64_t user_id, uint64_t subject_id) const override;

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

    static expertise_level to_level(std::string_view const level)
    {
        expertise_level result{};

        if (level == "surface") result = expertise_level::surface;
        else if (level == "depth") result = expertise_level::depth;
        else if (level == "origin") result = expertise_level::origin;
        else throw std::runtime_error{"invalid expertise level"};

        return result;
    }

    static std::string level_to_string(expertise_level const level)
    {
        std::string result{};

        switch (level)
        {
        case flashback::expertise_level::surface: result = "surface";
            break;
        case flashback::expertise_level::depth: result = "depth";
            break;
        case flashback::expertise_level::origin: result = "origin";
            break;
        default: throw std::runtime_error{"invalid expertise level"};
        }

        return result;
    }

    static std::string resource_type_to_string(Resource::resource_type const type)
    {
        std::string type_string{};

        switch (type)
        {
        case Resource::book: type_string = "book";
            break;
        case Resource::website: type_string = "website";
            break;
        case Resource::course: type_string = "course";
            break;
        case Resource::video: type_string = "video";
            break;
        case Resource::channel: type_string = "channel";
            break;
        case Resource::mailing_list: type_string = "mailing_list";
            break;
        case Resource::manual: type_string = "manual";
            break;
        case Resource::slides: type_string = "slides";
            break;
        case Resource::nerve: type_string = "nerve";
            break;
        default: throw std::runtime_error("invalid resource type");
        }

        return type_string;
    }

    static Resource::resource_type to_resource_type(std::string_view const type_string)
    {
        Resource::resource_type type{};

        if (type_string == "book") type = Resource::book;
        else if (type_string == "website") type = Resource::website;
        else if (type_string == "course") type = Resource::course;
        else if (type_string == "video") type = Resource::video;
        else if (type_string == "channel") type = Resource::channel;
        else if (type_string == "mailing_list") type = Resource::mailing_list;
        else if (type_string == "manual") type = Resource::manual;
        else if (type_string == "slides") type = Resource::slides;
        else if (type_string == "nerve") type = Resource::nerve;
        else throw std::runtime_error("invalid resource type");

        return type;
    }

    static std::string section_pattern_to_string(Resource::section_pattern const pattern)
    {
        std::string pattern_string{};

        switch (pattern)
        {
        case Resource::chapter: pattern_string = "chapter";
            break;
        case Resource::page: pattern_string = "page";
            break;
        case Resource::session: pattern_string = "session";
            break;
        case Resource::episode: pattern_string = "episode";
            break;
        case Resource::playlist: pattern_string = "playlist";
            break;
        case Resource::post: pattern_string = "post";
            break;
        case Resource::synapse: pattern_string = "synapse";
            break;
        default: throw std::runtime_error("invalid section pattern");
        }

        return pattern_string;
    }

    static Resource::section_pattern to_section_pattern(std::string_view const pattern_string)
    {
        Resource::section_pattern pattern{};

        if (pattern_string == "chapter") pattern = Resource::chapter;
        else if (pattern_string == "page") pattern = Resource::page;
        else if (pattern_string == "session") pattern = Resource::session;
        else if (pattern_string == "episode") pattern = Resource::episode;
        else if (pattern_string == "playlist") pattern = Resource::playlist;
        else if (pattern_string == "post") pattern = Resource::post;
        else if (pattern_string == "synapse") pattern = Resource::synapse;
        else throw std::runtime_error("invalid section pattern");

        return pattern;
    }

    static Card::card_state to_card_state(std::string_view const state_string)
    {
        Card::card_state state{};

        if (state_string == "draft") state = Card::draft;
        else if (state_string == "reviewed") state = Card::reviewed;
        else if (state_string == "completed") state = Card::completed;
        else if (state_string == "approved") state = Card::approved;
        else if (state_string == "released") state = Card::released;
        else if (state_string == "rejected") state = Card::rejected;
        else throw std::runtime_error("invalid card state");

        return state;
    }

    static std::string card_state_to_string(Card::card_state const state)
    {
        std::string state_string{};

        switch (state)
        {
        case Card::draft: state_string = "draft";
            break;
        case Card::reviewed: state_string = "reviewed";
            break;
        case Card::completed: state_string = "completed";
            break;
        case Card::approved: state_string = "approved";
            break;
        case Card::released: state_string = "released";
            break;
        case Card::rejected: state_string = "rejected";
            break;
        default: throw std::runtime_error("invalid card state");
        }

        return state_string;
    }

    static Block::content_type to_content_type(std::string_view const type_string)
    {
        Block::content_type type{};

        if (type_string == "code") type = Block::code;
        else if (type_string == "text") type = Block::text;
        else if (type_string == "image") type = Block::image;
        else throw std::runtime_error("invalid card state");

        return type;
    }

    static std::string content_type_to_string(Block::content_type const type)
    {
        std::string type_string{};

        switch (type)
        {
        case Block::code: type_string = "code";
            break;
        case Block::text: type_string = "text";
            break;
        case Block::image: type_string = "image";
            break;
        default: throw std::runtime_error("invalid content type");
        }

        return type_string;
    }

private:
    std::unique_ptr<pqxx::connection> m_connection;
};

} // flashback
