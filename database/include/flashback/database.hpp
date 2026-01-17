#pragma once

#include <pqxx/pqxx>
#include <server.grpc.pb.h>
#include <flashback/basic_database.hpp>

namespace flashback
{
class database: public basic_database
{
public:
    explicit database(std::string name = "flashback", std::string address = "localhost", std::string port = "5432");
    ~database() override = default;

    // users
    bool create_session(uint64_t user_id, std::string_view token, std::string_view device) const override;
    [[nodiscard]] uint64_t create_user(std::string_view name, std::string_view email, std::string_view hash) const override;
    void reset_password(uint64_t user_id, std::string_view hash) const override;
    [[nodiscard]] bool user_exists(std::string_view email) const override;
    [[nodiscard]] std::unique_ptr<User> get_user(std::string_view email) const override;
    [[nodiscard]] std::unique_ptr<User> get_user(std::string_view token, std::string_view device) const override;
    void revoke_session(uint64_t user_id, std::string_view token) const override;
    void revoke_sessions_except(uint64_t user_id, std::string_view token) const override;

    // roadmaps
    [[nodiscard]] Roadmap create_roadmap(uint64_t const user_id, std::string name) const override;
    [[nodiscard]] std::vector<Roadmap> get_roadmaps(uint64_t user_id) const override;
    void rename_roadmap(uint64_t roadmap_id, std::string_view modified_name) const override;
    void remove_roadmap(uint64_t roadmap_id) const override;
    [[nodiscard]] std::vector<Roadmap> search_roadmaps(std::string_view token) const override;

    // subjects
    [[nodiscard]] Subject create_subject(std::string name) const override;
    [[nodiscard]] std::map<uint64_t, Subject> search_subjects(std::string name) const override;
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
        case Resource::user: type_string = "user";
            break;
        default: throw std::runtime_error("invalid resource type");
        }

        return type_string;
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

private:
    std::unique_ptr<pqxx::connection> m_connection;
};
} // flashback
