#pragma once

#include <pqxx/pqxx>
#include <boost/asio.hpp>
#include <string>

namespace flashback
{
class database
{
private:
    pqxx::connection connection;
public:
    explicit database(std::string address = "localhost", std::string port = "5432");
};
} // flashback
