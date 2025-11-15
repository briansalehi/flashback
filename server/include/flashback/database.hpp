#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <pqxx/pqxx>

namespace flashback
{
class database
{
public:
    explicit database(std::string address = "localhost", std::string port = "5432");
    [[nodiscard]] pqxx::result query(std::string_view statement) const;
    void exec(std::string_view statement) const;

private:
    std::unique_ptr<pqxx::connection> m_connection;
};
} // flashback
