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

    bool create_session(uint64_t user_id, std::string_view token, std::string_view device) override;
    [[nodiscard]] uint64_t create_user(std::string_view name, std::string_view email, std::string_view hash) override;
    void reset_password(uint64_t user_id, std::string_view hash) override;
    [[nodiscard]] bool user_exists(std::string_view email) override;
    [[nodiscard]] std::unique_ptr<User> get_user(std::string_view email) override;
    [[nodiscard]] std::unique_ptr<User> get_user(uint64_t user_id, std::string_view device) override;
    void revoke_session(uint64_t user_id, std::string_view token) override;
    void revoke_sessions_except(uint64_t user_id, std::string_view token) override;
    [[nodiscard]] std::shared_ptr<GetRoadmapsResponse> get_roadmaps(uint64_t user_id) override;

private:
    [[nodiscard]] pqxx::result query(std::string_view statement);
    void exec(std::string_view statement);

    std::unique_ptr<pqxx::connection> m_connection;
};
} // flashback
