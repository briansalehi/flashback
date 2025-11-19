#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <utility>
#include <pqxx/pqxx>
#include <types.pb.h>

namespace flashback
{
class client_exception final : public std::exception
{
    std::string m_reason;
    std::string m_code;

public:
    client_exception(std::string reason)
        : m_reason{std::move(reason)}
    {
    }

    [[nodiscard]] const char* what() const noexcept override
    {
        return m_reason.c_str();
    }

    [[nodiscard]] std::string code() const noexcept
    {
        return m_code;
    }
};

class database
{
public:
    explicit database(std::string address = "localhost", std::string port = "5432");

    [[nodiscard]] std::shared_ptr<Roadmaps> get_roadmaps(uint64_t user_id);

    [[nodiscard]] std::pair<bool, std::string> create_session(uint64_t user_id, std::string_view token,
                                                              std::string_view device);
    [[nodiscard]] std::unique_ptr<User> get_user(std::string_view email);

private:
    [[nodiscard]] pqxx::result query(std::string_view statement);
    void exec(std::string_view statement);

    std::unique_ptr<pqxx::connection> m_connection;
};
} // flashback
