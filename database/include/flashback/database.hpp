#pragma once

#include <pqxx/pqxx>
#include <server.grpc.pb.h>
#include <flashback/basic_database.hpp>

namespace flashback
{
class database : public basic_database
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
    [[nodiscard]] std::unique_ptr<User> get_user(uint64_t user_id, std::string_view device) override;
    void revoke_session(uint64_t user_id, std::string_view token) override;
    void revoke_sessions_except(uint64_t user_id, std::string_view token) override;

    // roadmaps
    uint64_t create_roadmap(std::string_view name) override;
    void assign_roadmap_to_user(uint64_t user_id, uint64_t roadmap_id) override;
    [[nodiscard]] std::vector<Roadmap> get_roadmaps(uint64_t user_id) override;

private:
    template <typename... Args>
    [[nodiscard]] pqxx::result query(std::string_view format, Args&&... args) const
    {
        pqxx::work work{*m_connection};
        pqxx::result result{work.exec(format, pqxx::params{std::forward<Args>(args)...})};
        work.commit();
        return result;
    }

    template <typename... Args>
    void exec(std::string_view format, Args&&... args) const
    {
        pqxx::work work{*m_connection};
        work.exec(format, pqxx::params{std::forward<Args>(args)...});
        work.commit();
    }

    std::unique_ptr<pqxx::connection> m_connection;
};
} // flashback
