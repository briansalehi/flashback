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
    bool create_session(uint64_t user_id, std::string_view token, std::string_view device) override;
    [[nodiscard]] uint64_t create_user(std::string_view name, std::string_view email, std::string_view hash) override;
    void reset_password(uint64_t user_id, std::string_view hash) override;
    [[nodiscard]] bool user_exists(std::string_view email) override;
    [[nodiscard]] std::unique_ptr<User> get_user(std::string_view email) override;
    [[nodiscard]] std::unique_ptr<User> get_user(std::string_view token, std::string_view device) override;
    void revoke_session(uint64_t user_id, std::string_view token) override;
    void revoke_sessions_except(uint64_t user_id, std::string_view token) override;

    // roadmaps
    Roadmap create_roadmap(std::string name) override;
    void assign_roadmap(uint64_t user_id, uint64_t roadmap_id) override;
    [[nodiscard]] std::vector<Roadmap> get_roadmaps(uint64_t user_id) override;
    void rename_roadmap(uint64_t roadmap_id, std::string_view modified_name) override;
    void remove_roadmap(uint64_t roadmap_id) override;
    [[nodiscard]] std::vector<Roadmap> search_roadmaps(std::string_view token) override;

    // subjects
    Subject create_subject(std::string name) override;
    std::map<uint64_t, Subject> search_subjects(std::string name) override;
    void rename_subject(uint64_t id, std::string name) override;

    // milestones
    Milestone add_milestone(uint64_t subject_id, expertise_level subject_level, uint64_t roadmap_id) const override;
    Milestone add_milestone(uint64_t subject_id, expertise_level subject_level, uint64_t roadmap_id, uint64_t position) const override;
    std::vector<Milestone> get_milestones(uint64_t roadmap_id) const override;
    void add_requirement(uint64_t roadmap_id, uint64_t subject_id, expertise_level level, uint64_t required_subject_id, expertise_level minimum_level) const override;
    std::vector<Milestone> get_requiremnts(uint64_t roadmap_id, uint64_t subject_id) const override;

    // practices
    expertise_level get_user_cognitive_level(uint64_t user_id, uint64_t subject_id) const override;

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

    static expertise_level to_level(std::string_view level)
    {
        expertise_level result{};

        if (level == "surface") result = expertise_level::surface;
        else if (level == "depth") result = expertise_level::depth;
        else if (level == "origin") result = expertise_level::origin;
        else throw std::runtime_error{"invalid expertise level"};

        return result;
    }

    static std::string level_to_string(expertise_level level)
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

private:
    std::unique_ptr<pqxx::connection> m_connection;
};
} // flashback
