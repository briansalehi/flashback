#pragma once

#include <string>
#include <pqxx/pqxx>

namespace flashback
{
class database
{
public:
    database(std::string address, std::string port);

private:
    pqxx::connection m_connection;
};

} // flashback
