#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <utility>
#include <pqxx/pqxx>
#include <types.pb.h>

namespace flashback
{
class database
{
public:
    explicit database(std::string address = "localhost", std::string port = "5432");

    [[nodiscard]] std::shared_ptr<Roadmaps> get_roadmaps(uint64_t user_id);
    [[nodiscard]] std::pair<bool, std::string> create_session(uint64_t user_id, std::string_view token,
                                                              std::string_view device);
    [[nodiscard]] uint64_t create_user(std::string_view name, std::string_view email, std::string_view hash);
    void reset_password(uint64_t user_id, std::string_view hash);
    [[nodiscard]] std::unique_ptr<User> get_user(std::string_view email);
    [[nodiscard]] std::unique_ptr<User> get_user(std::string_view email, std::string_view device);

private:
    [[nodiscard]] pqxx::result query(std::string_view statement);
    void exec(std::string_view statement);

    std::unique_ptr<pqxx::connection> m_connection;
};
} // flashback
